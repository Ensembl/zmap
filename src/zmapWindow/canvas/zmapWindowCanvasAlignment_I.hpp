/*  File: zmapWindowCanvasAlignment_I.h
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
 * implements callback functions for FeaturesetItem alignment features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_ALIGNMENT_I_H
#define ZMAP_CANVAS_ALIGNMENT_I_H

#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasBasic.hpp>
#include <zmapWindowCanvasGlyph_I.hpp>
#include <zmapWindowCanvasAlignment.hpp>



#define N_ALIGN_GAP_ALLOC	1000



typedef struct _zmapWindowCanvasAlignmentStruct
{
  zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */

  /* NOTE that we can have: alignment.feature->feature->feature.homol */

  AlignGap gapped;		/* boxes and lines to draw when bumped */

  /* stuff for displaying homology status derived from feature data when loading */

  /* we display one glyph max at each end
   * so far these can be 2 kinds of homology incomplete or nc-splice, which cannot overlap
   * however these are defined as sub styles and we can have more types in diff featuresets
   * so we cache these in global hash table indexed by a glyph signature
   * zero means don't display a glyph
   */
  ZMapWindowCanvasGlyph glyph5 ;
  ZMapWindowCanvasGlyph glyph3 ;

  /* has homology and gaps data (lazy evaluation) */
  gboolean bump_set ;


} zmapWindowCanvasAlignmentStruct, *ZMapWindowCanvasAlignment;




#endif /* !ZMAP_CANVAS_ALIGNMENT_I_H */


