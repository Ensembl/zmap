/*  File: zmapWindowContainerFeatureSet_I.h
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
 * Description: Internal header for class that represents a column
 *              in zmap.
 *              Note that a column may contain zero or more featuresets,
 *              each featureset is 
 *
 *-------------------------------------------------------------------
 */
#ifndef __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__
#define __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__

#include <glib.h>
#include <zmapWindowContainerGroup_I.hpp>
#include <zmapWindowContainerFeatureSet.hpp>


/* This all needs renaming to be xxxxColumnxxxxx, not "featureset".... */



/* 
 *           Class struct.
 */

typedef struct _zmapWindowContainerFeatureSetClassStruct
{
  zmapWindowContainerGroupClass __parent__ ;

} zmapWindowContainerFeatureSetClassStruct ;



/* 
 *           Instance struct.
 */

typedef struct _zmapWindowContainerFeatureSetStruct
{
  zmapWindowContainerGroup __parent__ ;

  ZMapStyleColumnDisplayState display_state ;

  gboolean                    has_stats ;

  ZMapWindow  window;
  ZMapStrand  strand ;
  ZMapFrame   frame ;


  /* style & bump_mode are both used by it's not clear for what.... */


  /* HOW DO THESE TWO RELATE TO THE STUFF IN THE canvasfeatureset struct which seems to replicate
   * these fields ???? je ne sais pas..... */
  /* this is a column setting, the settings struct below is an amalgamation of styles */
  ZMapStyleBumpMode bump_mode ;

  ZMapFeatureTypeStyle style ;       /* column specific style or the one single style for a
                                        featureset */


  /* Empty columns are only hidden ATM and as they have no
   * ZMapFeatureSet removing them from the FToI hash becomes difficult
   * without the align, block and set ids. No doubt it'l be true for
   * empty block and align containers too at some point. */

  GQuark      align_id ;
  GQuark      block_id ;
  GQuark      unique_id ;
  GQuark      original_id ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Never referenced anywhere.... */

  /* We keep the features sorted by position and size so we can cursor through them... */
  gboolean    sorted ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* does the column contain featuresets that are maskable? */
  gboolean maskable ;

  /* does the column contain masked featuresets that have been masked (not displayed)? (default) */
  gboolean masked ;



  gboolean has_feature_set ;


  /* list of ZMapFeatureset ids displayed in this column, this is not the ZMapWindowFeaturesetItem
   * for each featureset....AGH....this is so hard to follow.....
   * 
   * HORRIFICALLY THIS IS ACCESSED DIRECTLY AND POPULATED IN zmapWindowDrawFeatures.c....
   *  */
  GList *featuresets ;


  /* Features hidden for filtering by user. */
  gboolean col_filter_sensitive ;
  ZMapWindowContainerFilterType curr_selected_type ;
  ZMapWindowContainerFilterType curr_filter_type ;
  ZMapWindowContainerActionType curr_filter_action ;
  gboolean cds_match ;
  GList *filtered_features ;


  /* Features hidden by user, should stay hidden until unhidden by user. */
  GQueue *user_hidden_stack ;

  /* These fields are used for some of the more exotic column bumping. */
  gboolean hidden_bump_features ;                           /* Features were hidden because they
                                                             * are out of the marked range. *//*  */


} zmapWindowContainerFeatureSetStruct ;




#endif /* __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__ */
