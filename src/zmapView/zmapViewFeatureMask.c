/*  File: zmapViewFeatureMask.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:   masks EST features with mRNAs subject to configuration
 *                NOTE see Design_notes/notes/EST_mRNA.shtml
 *                sets flags in the context per feature to say masked or not
 *                that display code can use
 *
 * Created: Fri Jul 23 2010 (mh17)
 * CVS info:   $Id: zmapViewFeatureMask.c,v 1.1 2010-07-23 14:48:25 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtils.h>
#include <zmapView_P.h>


/*
 * for masking ESTs by mRNAs we first merge in the new featuresets
 * then for each new featureset we determine if it masks another featureset
 * and store this info somewhere
 * then if there are any featuresets to mask we process each one in turn
 * each featureset masking operation is 1-1 and we only mask with new data
 * masking code just works on the merged context and a list of featuresets
 */

/*
- identify new masking (mRNA) and new masked featuresets (EST) (as  configured)
  - new masker featuresets are applied to old masked data
  - new masked featuresets are masked by all relevant ones

- sort the mRNA's into start coordinate order, then end coordinate reversed
- remove or flag mRNAs that are covered by another one - this will be easy as they are in order
- sort the ESTs into order as for the mRNAs and hide ESTs that are covered by others

- scan through EST's and for each one:
- scan all the mRNA's until the start coordinate means no cover
- skip over mRNA's whose end coord means no cover
  - this can be optimised by including a pointer to the next feature with a different start coordinate
  - many features have the same start coordinate.

- express both featuresets as integer arrays or lists defining exon and intron like sections.
- use a simple function to determine if one covers the other completely and if so hide that EST and quit.
 */




typedef struct _ZMapMaskFeatureSetData
{
      GList *masker;
      GList *masked;
      ZMapFeatureAny block;   // that contains the current featureset
      ZMapView *view;
      GQuark mask_me;         // current new featureset for masking
}
ZMapMaskFeatureSetDataStruct, *ZMapMaskFeatureSetData;


static ZMapFeatureContextExecuteStatus maskOldFeaturesetByNew(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
static ZMapFeatureContextExecuteStatus maskNewFeaturesetByAll(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)

void mask_set_with_set(ZMapFeatureSet masked, ZMapFeatureSet masker);



// mask ESTs with mRNAs if configured
// all new features are already in view->context
// logically we can only mask features in the same block
//NOTE even tho' masker sets are configure that does not mean we have the data yet

void zMapFeatureMaskFeatureSets(ZMapView view, GList *feature_set_names)
{
      ZMapMaskFeatureSetData data = { NULL };
      GList *fset;
      ZMapGFFSource src2src;
      ZMapFeatureTypeStyle style;
      GList *masked_by;

      for(fset = feature_set_names;fset;fset = fset->next)
      {
            src2src = (ZMapGFFSource) g_hash_table_lookup(view->source_2_sourcedata,fset->data);
            if(!src2src)
            {
                  zMapLogWarning("zMapFeatureMaskFeatureSets() cannot find style for %s", g_quark_to_string(GPOINTER_TO_UINT(fset->data));
                  continue;
            }
            style = src2src->style;

            if((masked_by = zMapStyleGetMaskList(style))
            {
                  data->masked = g_list_prepend(data->masked,fset->data);
            }
            else
            {
                  data->masker = g_list_prepend(data->masker,fset->data);
            }
      }

      if(fset = masked;fset;fset = fset->next)      // mask new featuresets by all
      {
            // with pipes we expect 1 featureset at a time but from ACE all 40 or so at once
            // so we do one ContextExecute for each one
            // which is OTT but we get thro' the aligns and blocks very quickly
            // and then the masker featuresets are found in the block's hash table
            // so really it's quite speedy

            data->mask_me = fset->data;
            zMapFeatureContextExecuteFull((ZMapFeatureAny) view->context
                                   ZMAPFEATURE_STRUCT_BLOCK,
                                   maskNewFeaturesetByAll,
                                   (gpointer) data);
      }

      if(masker)                                    // mask old featuresets by new data
      {
            zMapFeatureContextExecuteFull((ZMapFeatureAny) view->context
                                   ZMAPFEATURE_STRUCT_FEATURESET,
                                   maskOldFeaturesetByNew,
                                   (gpointer) data);
      }
}



// only mask old featureset with new data in same block
static ZMapFeatureContextExecuteStatus maskOldFeaturesetByNew(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapMaskFeatureSetData cb_data = (ZMapMaskFeatureSetData) user_data;
  GList *fset;
  GList *masked_by;
  ZMapGFFSource src2src;
  ZMapFeatureTypeStyle style;

  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;
        feature_block  = (ZMapFeatureBlock)feature_any;
        cb_data->block = feature_block;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;
        ZMapFeatureSet masker_set = NULL;
        feature_set = (ZMapFeatureSet)feature_any;          // old featuresset

        src2src = (ZMapGFFSource) g_hash_table_lookup(cb_data->view->source_2_sourcedata, feature_any->unique_id);
        zMapAssert(src2src);

        style = src2src->style;
        masked_by = zMapStyleGetMaskList(style);            // all the masker featuresets

        for(fset = masked_by;fset;fset = fset->next)
        {
            // with pipes we expect one featureset at a time in which case this is trivial
            // with 40 featuresets from ACE we could have a large worst case
            // but realistically we expect only 2-3 masker featuresets
            // so it's not a performance problem despite the naive code

            GList *masker = g_list_find(cb_data->masker,fset->data);

            if(masker)                                      // only mask if new masker
            {
                  GList *new = g_list_find(cb_data->masked,fset->data);

                  if(!new)                                  // only mask if data was old
                  {
                        masker_set = (ZMapFeatureSet)
                               g_hash_table_lookup(cb_data->block->feature_sets,fset->data);
                        if(masker_set)
                              mask_set_with_set(feature_set,masker_set);
                  }
            }
        }
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
      zMapAssertNotReached();
      break;
      }
    }

  return status;
}



// mask new featureset with all (old+new) data in same block
static ZMapFeatureContextExecuteStatus maskNewFeaturesetByAll(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapMaskFeatureSetData cb_data = (ZMapMaskFeatureSetData) user_data;
  GList *fset;
  GList *masked_by;
  ZMapGFFSource src2src;

  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;
        ZMapFeatureSet feature_set = NULL;
        ZMapFeatureSet masker_set = NULL;
//        feature_block  = (ZMapFeatureBlock)feature_any;
//        cb_data->block = feature_block;

            // do we have our target featureset in this block?
        feature_set = (ZMapFeatureSet) g_hash_table_lookup(feature_any->children, GUINT_TO_POINTER(cb_data->mask_me));
        if(!feature_set)
            break;

        src2src = (ZMapGFFSource) g_hash_table_lookup(cb_data->view->source_2_sourcedata, feature_any->unique_id);
        zMapAssert(src2src);

        style = src2src->style;
        masked_by = zMapStyleGetMaskList(style);            // all the masker featuresets

        for(fset = masked_by;fset;fset = fset->next)
        {
            masker_set = (ZMapFeatureSet) g_hash_table_lookup(feature_any->children, fset->data);

            if(masker_set)
                  mask_set_with_set(feature_set, masker_set);
        }
      }
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;
        feature_set = (ZMapFeatureSet)feature_any;
      }
      // fall through
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
      zMapAssertNotReached();
      break;
      }
    }

  return status;
}


void mask_set_with_set(ZMapFeatureSet masked, ZMapFeatureSet masker)
{
      printf("mask %s with new %s\n", g_quark_to_string(masked->original_id), g_quark_to_string(masker->original_id));
}