/*  File: zmapWindowCanvasAssembly.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * implements callback functions for FeaturesetItem assembly features
 * NOTE originally assemebly feature has display option to shade overlaps and show the actual path chosen
 * this was never immplemented in ZMapWindowCanvasItems and this mopdule is initially a copy of CanvasBasic
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasAssembly_I.hpp>


/* not static as we want to use this in alignments */
void zMapWindowCanvasAssemblyPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                          GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong ufill,outline;
  int colours_set, fill_set, outline_set;
  double x1,x2;

  zMapReturnIfFail(featureset && feature && drawable && expose) ;

  /* draw a box */

  /* colours are not defined for the CanvasFeatureSet
   * as we can have several styles in a column
   * but they are cached by the calling function
   * and also the window focus code
   */

  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &ufill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  if (zMapWindowCanvasCalcHorizCoords(featureset, feature, &x1, &x2))
    zMapCanvasFeaturesetDrawBoxMacro(featureset,
                                     x1, x2, feature->y1, feature->y2,
                                     drawable, fill_set, outline_set, ufill,outline) ;

  return ;
}



void zMapWindowCanvasAssemblyInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_PAINT] = (void *)zMapWindowCanvasAssemblyPaintFeature;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ASSEMBLY, funcs, (size_t)0) ;

  zMapWindowCanvasFeatureSetSize(FEATURE_ASSEMBLY, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;

  return ;
}

