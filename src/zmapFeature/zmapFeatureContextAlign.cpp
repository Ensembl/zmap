/*  File: zmapFeatureContextAlign.cpp
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
 * Description: The Alignment feature is the child of the context,
 *              it contains blocks which in turn contain featuresets.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <zmapFeature_P.hpp>




/*
 *                External interface functions
 */

GQuark zMapFeatureAlignmentCreateID(char *align_name, gboolean master_alignment)
{
  GQuark id = 0;
  char *unique_name;

  if (master_alignment)
    unique_name = g_strdup_printf("%s_master", align_name) ;
  else
    unique_name = g_strdup(align_name) ;

  id = g_quark_from_string(unique_name) ;

  g_free(unique_name);

  return id;
}


ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name, gboolean master_alignment)
{
  ZMapFeatureAlignment alignment = NULL ;

  if (align_name && *align_name)
    {
      GQuark unique_id ;

      unique_id = zMapFeatureAlignmentCreateID(align_name, master_alignment) ;

      alignment = (ZMapFeatureAlignment)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_ALIGN,
                                                                    NULL,
                                                                    g_quark_from_string(align_name),
                                                                    unique_id,
                                                                    NULL) ;
    }

  return alignment ;
}

/* If aligment sequence name is of the form "chr6-18" returns "6" otherwise NULL. */
char *zMapFeatureAlignmentGetChromosome(ZMapFeatureAlignment feature_align)
{
  char *chromosome_text = NULL ;
  char *search_str ;

  search_str = (char *)g_quark_to_string(feature_align->original_id) ;

  zMapUtilsBioParseChromNumber(search_str, &chromosome_text) ;

  return chromosome_text ;
}


gboolean zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment alignment, ZMapFeatureBlock block)
{
  gboolean result = FALSE  ;

  if (!alignment || !block)
    return result ;

  result = zmapFeatureAnyAddFeature((ZMapFeatureAny)alignment, (ZMapFeatureAny)block) ;

  if (result)
    {
      /* remember where our data hails from */
      if (!alignment->sequence_span.x1 || alignment->sequence_span.x1 > block->block_to_sequence.block.x1)
        alignment->sequence_span.x1 = block->block_to_sequence.block.x1;

      if (!alignment->sequence_span.x2 || alignment->sequence_span.x2 < block->block_to_sequence.block.x2)
        alignment->sequence_span.x2 = block->block_to_sequence.block.x2;
    }

  return result ;
}


gboolean zMapFeatureAlignmentFindBlock(ZMapFeatureAlignment feature_align,
                                       ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  zMapReturnValIfFail(feature_align && feature_block, result) ;

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_align, (ZMapFeatureAny)feature_block) ;

  return result;
}

ZMapFeatureBlock zMapFeatureAlignmentGetBlockByID(ZMapFeatureAlignment feature_align, GQuark block_id)
{
  ZMapFeatureBlock feature_block = NULL;

  feature_block = (ZMapFeatureBlock)zMapFeatureParentGetFeatureByID((ZMapFeatureAny)feature_align, block_id) ;

  return feature_block ;
}


gboolean zMapFeatureAlignmentRemoveBlock(ZMapFeatureAlignment feature_align,
                                         ZMapFeatureBlock     feature_block)
{
  gboolean result = FALSE;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_align, (ZMapFeatureAny)feature_block) ;

  return result;
}

void zMapFeatureAlignmentDestroy(ZMapFeatureAlignment alignment, gboolean free_data)
{
  if (alignment)
    {
      zmapDestroyFeatureAnyWithChildren((ZMapFeatureAny)alignment, free_data) ;
    }

  return ;
}





