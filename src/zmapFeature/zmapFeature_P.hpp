/*  File: zmapFeature_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
