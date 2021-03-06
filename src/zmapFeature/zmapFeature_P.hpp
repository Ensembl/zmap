/*  File: zmapFeature_P.h
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
 * Description: Internals for zmapFeature routines.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_P_H
#define ZMAP_FEATURE_P_H

#include <ZMap/zmapFeature.hpp>



/* Used to construct a table of strings and corresponding enums to go between the two,
 * the table must end with an entry in which type_str is NULL. */
typedef struct
{
  char *type_str ;
  int type_enum ;
} ZMapFeatureStr2EnumStruct, *ZMapFeatureStr2Enum ;



#define zmapFeature2HashKey(FEATURE_ANY)  \
  GINT_TO_POINTER((FEATURE_ANY)->unique_id)



ZMapFeatureAny zmapFeatureAnyCreateFeature(ZMapFeatureLevelType feature_type,
                                           ZMapFeatureAny parent,
                                           GQuark original_id, GQuark unique_id,
                                           GHashTable *children) ;
ZMapFeatureAny zmapFeatureAnyCopy(ZMapFeatureAny orig_feature_any, GDestroyNotify destroy_cb) ;
void zmapDestroyFeatureAny(gpointer data) ;
gboolean zmapFeatureAnyAddFeature(ZMapFeatureAny feature_any, ZMapFeatureAny feature) ;
gboolean zmapDestroyFeatureAnyWithChildren(ZMapFeatureAny feature_any, gboolean free_children) ;
void zmapFeatureAnyAddToDestroyList(ZMapFeatureContext context, ZMapFeatureAny feature_any) ;
void zmapDestroyFeatureAny(gpointer data) ;

void zmapFeatureRevComp(const int seq_start, const int seq_end, int *coord1, int *coord2) ;
void zmapFeatureInvert(int *coord, const int seq_start, const int seq_end) ;

void zmapPrintFeatureContext(ZMapFeatureContext context) ;




void zmapFeatureBlockAddEmptySets(ZMapFeatureBlock ref, ZMapFeatureBlock block, GList *feature_set_names) ;



void zmapFeature3FrameTranslationSetRevComp(ZMapFeatureSet feature_set, int block_start, int block_end) ;
void zmapFeatureORFSetRevComp(ZMapFeatureSet feature_set, ZMapFeatureSet translation_fs) ;

int zmapFeatureDNACalculateVariationDiff(const int start, 
                                         const int end,
                                         GList *variations) ;

gboolean zmapFeatureCoordsMatch(int slop,  gboolean full_match,
                                int boundary_start, int boundary_end, int start, int end,
                                int *match_out, int *end_inout) ;
gboolean zmapFeatureMatchingBoundaries(ZMapFeature feature,
                                       gboolean exact_match, int slop,
                                       ZMapFeaturePartsList boundaries,
                                       ZMapFeaturePartsList *matching_boundaries_out,
                                       ZMapFeaturePartsList *non_matching_boundaries_out) ;

ZMapFeaturePartsList zmapFeatureBasicSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds) ;
gboolean zmapFeatureBasicMatchingBoundaries(ZMapFeature feature,
                                            ZMapFeatureSubPartType part_type, gboolean exact_match, int slop,
                                            ZMapFeaturePartsList boundaries,
                                            ZMapFeaturePartsList *matching_boundaries_out,
                                            ZMapFeaturePartsList *non_matching_boundaries_out) ;
void zmapFeatureBasicCopyFeature(ZMapFeature orig_feature, ZMapFeature new_feature) ;
void zmapFeatureBasicDestroyFeature(ZMapFeature feature) ;

ZMapFeaturePartsList zmapFeatureAlignmentSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds) ;
gboolean zmapFeatureAlignmentMatchingBoundaries(ZMapFeature feature,
                                                ZMapFeatureSubPartType part_type, gboolean exact_match, int slop,
                                                ZMapFeaturePartsList boundaries,
                                                ZMapFeaturePartsList *matching_boundaries_out,
                                                ZMapFeaturePartsList *non_matching_boundaries_out) ;
void zmapFeatureAlignmentCopyFeature(ZMapFeature orig_feature, ZMapFeature new_feature) ;
void zmapFeatureAlignmentDestroyFeature(ZMapFeature feature) ;

ZMapFeaturePartsList zmapFeatureTranscriptSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds) ;
gboolean zmapFeatureTranscriptMatchingBoundaries(ZMapFeature feature,
                                                 ZMapFeatureSubPartType part_type, gboolean exact_match, int slop,
                                                 ZMapFeaturePartsList boundaries,
                                                 ZMapFeaturePartsList *matching_boundaries_out,
                                                 ZMapFeaturePartsList *non_matching_boundaries_out) ;
void zmapFeatureTranscriptCopyFeature(ZMapFeature orig_feature, ZMapFeature new_feature) ;
void zmapFeatureTranscriptDestroyFeature(ZMapFeature feature) ;


// we surely need to get rid of this.....   
gboolean zmapStr2Enum(ZMapFeatureStr2Enum type_table, char *type_str, int *type_out) ;


#endif /* !ZMAP_FEATURE_P_H */
