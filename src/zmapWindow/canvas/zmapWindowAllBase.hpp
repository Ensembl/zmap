/*  File: zmapWindowAllBase.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: There are some things common to all groups and items
 *              and this file contains them.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_ALL_BASE_H
#define ZMAP_WINDOW_ALL_BASE_H

#include <libzmapfoocanvas/libfoocanvas.h>


// SHOULD THESE GO INTO THE ALIGNMENT CODE...PERHAPS NOT, MIGHT BE USED IN
// TRANSCRIPT CODE IN THE END.

/* currently needed in alignmentfeatuer and featureset, if we stop using it in
 * alignment then it should go to the featureset internal header. */
/* Colours for matches to indicate degrees of colinearity. */
#define ZMAP_WINDOW_MATCH_NOTCOLINEAR   "red"
#define ZMAP_WINDOW_MATCH_COLINEAR      "orange"
#define ZMAP_WINDOW_MATCH_PERFECT       "green"


/* Used to specify the degree of colinearity between two alignment blocks. */
typedef enum
  {
    COLINEAR_INVALID,
    COLINEAR_NOT,                                          /* blocks not colinear. */
    COLINEAR_IMPERFECT,                                    /* blocks colinear but not contiguous. */
    COLINEAR_PERFECT,                                      /* blocks colinear and contiguous. */
    COLINEARITY_N_TYPE
  } ColinearityType ;



/*
 * Should add some of the stats stuff here....
 *
 *  */


/* Struct to hold basic stats for a particular zmap item. */
typedef struct ZMapWindowItemStatsType
{
  GType type ;						    /* GType for item. */
  char *name ;						    /* Text string name of item. */
  unsigned int size ;					    /* size in bytes of item struct. */
  unsigned int total ;					    /* Total objects created. */
  unsigned int curr ;					    /* Curr objects existing. */
} ZMapWindowItemStatsStruct, *ZMapWindowItemStats ;


GType zmapWindowItemTrueType(FooCanvasItem *item) ;
GType zmapWindowItemTrueTypePrint(FooCanvasItem *item) ;

void zmapWindowItemStatsInit(ZMapWindowItemStats stats, GType type) ;
void zmapWindowItemStatsIncr(ZMapWindowItemStats stats) ;
void zmapWindowItemStatsDecr(ZMapWindowItemStats stats) ;




#endif /* ZMAP_WINDOW_ALL_BASE_H */
