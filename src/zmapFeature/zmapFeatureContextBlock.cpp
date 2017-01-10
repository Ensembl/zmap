/*  File: zmapFeatureContextBlock.cpp
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Blocks are the children of aligns and are parents of
 *              featuresets. Each block is a contiguous sequence plus
 *              features.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <ZMap/zmapGLibUtils.hpp>
#include <zmapFeature_P.hpp>


// 
//                External routines
//    



/*
 *           External interface functions
 */


ZMapFeatureBlock zMapFeatureBlockCreate(char *block_seq,
                                        int ref_start, int ref_end, ZMapStrand ref_strand,
                                        int non_start, int non_end, ZMapStrand non_strand)
{
  ZMapFeatureBlock new_block = NULL ;

  if (!((ref_strand == ZMAPSTRAND_FORWARD || ref_strand == ZMAPSTRAND_REVERSE)
        && (non_strand == ZMAPSTRAND_FORWARD || non_strand == ZMAPSTRAND_REVERSE)) )
    return new_block ;

  new_block = (ZMapFeatureBlock)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_BLOCK,
                                                            NULL,
                                                            g_quark_from_string(block_seq),
                                                            zMapFeatureBlockCreateID(ref_start, ref_end, ref_strand,
                                                                                     non_start, non_end, non_strand),
                                                            NULL) ;

  new_block->block_to_sequence.parent.x1 = ref_start ;
  new_block->block_to_sequence.parent.x2 = ref_end ;
  //  new_block->block_to_sequence.t_strand = ref_strand ;

  new_block->block_to_sequence.block.x1 = non_start ;
  new_block->block_to_sequence.block.x2 = non_end ;
  //  new_block->block_to_sequence.q_strand = non_strand ;

  new_block->block_to_sequence.reversed = (ref_strand != non_strand);

  //  new_block->features_start = new_block->block_to_sequence.q1 ;
  //  new_block->features_end = new_block->block_to_sequence.q2 ;

  return new_block ;
}



/* was used to set request coord in feature_block we prefer to set these explicitly in the req protocol
 * howeverwe still need to set thes up for sub-sequence requests to handle 'no data'
 */
gboolean zMapFeatureBlockSetFeaturesCoords(ZMapFeatureBlock feature_block,
                                           int features_start, int features_end)
{
  gboolean result = FALSE  ;


  feature_block->block_to_sequence.parent.x1 = features_start ;
  feature_block->block_to_sequence.parent.x2 = features_end ;

  feature_block->block_to_sequence.block.x1 = features_start ;
  feature_block->block_to_sequence.block.x2 = features_end ;

  result = TRUE ;

  return result ;
}


gboolean zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block,
                                       ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE  ;

  if (!feature_block || !feature_set)
    return result ;

  result = zmapFeatureAnyAddFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result ;
}


gboolean zMapFeatureBlockFindFeatureSet(ZMapFeatureBlock feature_block,
                                        ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE ;

  if (!feature_set || !feature_block)
    return result ;

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result;
}

ZMapFeatureSet zMapFeatureBlockGetSetByID(ZMapFeatureBlock feature_block, GQuark set_id)
{
  ZMapFeatureSet feature_set ;

  feature_set = (ZMapFeatureSet)zMapFeatureParentGetFeatureByID((ZMapFeatureAny)feature_block, set_id) ;

  return feature_set ;
}

GList *zMapFeatureBlockGetMatchingSets(ZMapFeatureBlock feature_block, char *prefix)
{
  GList *sets = NULL,*s, *del;
  ZMapFeatureSet feature_set;

  zMap_g_hash_table_get_data(&sets, feature_block->feature_sets);

  for(s = sets; s; )
    {
      const char *name;

      feature_set = (ZMapFeatureSet) s->data;
      name = g_quark_to_string(feature_set->unique_id);

      del = s;
      s = s->next;

      if(!g_str_has_prefix(name, prefix))
        sets = g_list_delete_link(sets, del);
    }

  return sets;
}


gboolean zMapFeatureBlockRemoveFeatureSet(ZMapFeatureBlock feature_block,
                                          ZMapFeatureSet   feature_set)
{
  gboolean result = FALSE;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_block, (ZMapFeatureAny)feature_set) ;

  return result;
}


/* add emtpy sets to block when not present in ref, to record successful requests w/ no data */
void zmapFeatureBlockAddEmptySets(ZMapFeatureBlock ref, ZMapFeatureBlock block, GList *feature_set_names)
{
  GList *l;

#if MH17_DEBUG
  {
    ZMapFeatureAny feature_any = (ZMapFeatureAny) block;
    zMap_g_hash_table_get_keys(&l,feature_any->children);
    for(;l;l = l->next)
      {
        zMapLogWarning("block has featureset %s",g_quark_to_string(GPOINTER_TO_UINT(l->data)));
      }
  }
#endif

  for(l  = feature_set_names;l;l = l->next)
    {
      ZMapFeatureSet old_set;
      GQuark set_id = GPOINTER_TO_UINT(l->data);

      char *set_name = (char *) g_quark_to_string(set_id);
      set_id = zMapFeatureSetCreateID(set_name);

      if(!(old_set = zMapFeatureBlockGetSetByID(ref, set_id)))
        {
          if(!g_strstr_len(set_name,-1,":"))
            {
              /*
                NOTE (prefix)
                featuresets with a 'prefix:' name are known as stubs
                and are very numerous (eg 1000) and normally contain no data
                they come only from ACEDB and only update via XML from otterlace
                so if they have no data we ignore them and do not create them
                as ACEDB takes requests as columns then a subsequent request will work

                NOTE it would be good not to have these featuresets in the columns featureset list
                and also in the featureset to column list
              */

              ZMapFeatureSet feature_set;
              ZMapSpan span;

              feature_set = zMapFeatureSetCreate(set_name ,NULL);
              /* record the region we are getting */
              span = (ZMapSpan) g_new0(ZMapSpanStruct,1);
              span->x1 = block->block_to_sequence.block.x1;
              span->x2 = block->block_to_sequence.block.x2;
              feature_set->loaded = g_list_append(NULL,span);

              zMapFeatureBlockAddFeatureSet(block, feature_set);
            }
        }
    }
}


/* for most columns we just load the lot, so there will be a single range */
/* NOTE we find the featureset in the first align/block, as ATM there can only be one
 * the context is designed to have more and if so we need to supply a current block to this function
 * refer to 'Handling Multiple Blocks' in zmapWindowColConfig.html
 */
gboolean zMapFeatureSetIsLoadedInRange(ZMapFeatureBlock block,  GQuark unique_id,int start, int end)
{
  GList *l;
  ZMapFeatureSet fset;
  ZMapSpan span;

  fset = zMapFeatureBlockGetSetByID(block,unique_id);
  if(fset)
    {
      for(l = fset->loaded;l;l = l->next)
        {
          span = (ZMapSpan)(l->data);
          continue;

          if(span->x1 > start)
            return FALSE;

          if(!span->x2)     /* not real coordinates */
            {
              return TRUE ;
            }
          if(span->x2 >= end)
            return TRUE;

          start  = span->x2 + 1;
        }
    }
  else
    {
      /* see NOTE (prefix) below */
      //            printf("Featureset %s does not exist\n",g_quark_to_string(unique_id));
    }

  return FALSE;
}




void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data)
{
  if (!block)
    return ;

  zmapDestroyFeatureAnyWithChildren((ZMapFeatureAny)block, free_data) ;

  return ;
}

