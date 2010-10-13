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
 * CVS info:   $Id: zmapViewFeatureMask.c,v 1.4 2010-10-13 09:00:38 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtils.h>
//#include <ZMap/zmapGFF.h>
#include <zmapView_P.h>


/*
 * for masking ESTs by mRNAs we first merge in the new featuresets
 * then for each new featureset we determine if it masks another featureset
 * and store this info somewhere
 * then if there are any featuresets to mask we process each one in turn
 * each featureset masking operation is 1-1 and we only mask with new data
 * masking code just works on the merged context and a list of featuresets
 */

/*!
 *     NOTE: See Design_notes/notes/EST_mRNA.shtml for the write up
 */

#define FILE_DEBUG      0     /* can use this for performance stats, coould add it to the log? */
#define PDEBUG          printf      /* zMapLogWarning */


/* header of list of features of the same name */
typedef struct _align_set
{
      GQuark id;
      Coord x1,x2;
      gboolean masked;  /* by self */

} ZMapViewAlignSetStruct, *ZMapViewAlignSet;


typedef struct _ZMapMaskFeatureSetData
{
      GList *masker;
      GList *masked;
      ZMapFeatureBlock block; /* that contains the current featureset */
      ZMapView view;
      GQuark mask_me;         /* current new featureset for masking */
      GList *redisplay;       /* existing columns that get masked */
}
ZMapMaskFeatureSetDataStruct, *ZMapMaskFeatureSetData;


static ZMapFeatureContextExecuteStatus maskOldFeaturesetByNew(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out);
static ZMapFeatureContextExecuteStatus maskNewFeaturesetByAll(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out);

static void mask_set_with_set(ZMapFeatureSet masked, ZMapFeatureSet masker);

static GList *sortFeatureset(ZMapFeatureSet fset);

/* mask ESTs with mRNAs if configured
 * all new features are already in view->context
 * logically we can only mask features in the same block
 * NOTE even tho' masker sets are configured that does not mean we have the data yet
 *
 * returns a lists of previously displayed featureset id's that have been masked
 */
GList *zMapViewMaskFeatureSets(ZMapView view, GList *new_feature_set_names)
{
      ZMapMaskFeatureSetDataStruct _data = { NULL };
      ZMapMaskFeatureSetData data = &_data;
      GList *fset;
      ZMapFeatureSource src2src;
      ZMapFeatureTypeStyle style;
      GList *masked_by;

      data->view = view;

            /* this is the featuresets from the context not the display columns */
      for(fset = new_feature_set_names;fset;fset = fset->next)
      {
            src2src = (ZMapFeatureSource) g_hash_table_lookup(view->context_map.source_2_sourcedata,fset->data);
            if(!src2src)
            {
                  zMapLogWarning("zMapFeatureMaskFeatureSets() cannot find style id for %s", g_quark_to_string(GPOINTER_TO_UINT(fset->data)));
                  continue;
            }
            style =  zMapFindStyle(view->context_map.styles,src2src->style_id);
            if(!style)
            {
                  zMapLogWarning("zMapFeatureMaskFeatureSets() cannot find style for %s", g_quark_to_string(GPOINTER_TO_UINT(fset->data)));
                  continue;
            }


            if((masked_by = zMapStyleGetMaskList(style)))
            {
                  GList *l;
                  GQuark self = g_quark_from_string("self");
                  GQuark set_id = GPOINTER_TO_UINT(fset->data);
                  gboolean self_only = TRUE;

                  for(l = masked_by;l;l = l->next)
                  {
                        set_id = GPOINTER_TO_UINT(l->data);

                        if(set_id == self)
                              set_id = GPOINTER_TO_UINT(fset->data);
                        else
                              self_only = FALSE;

                        if(!g_list_find(data->masker,GUINT_TO_POINTER(set_id)))
                              data->masker = g_list_prepend(data->masker,GUINT_TO_POINTER(set_id));
                  }
                  if(self_only)     // prioritise these, likely they'll mask others
                        data->masked = g_list_prepend(data->masked,fset->data);
                  else
                        data->masked = g_list_append(data->masked,fset->data);
            }
      }

#if FILE_DEBUG
      if(data->masked) PDEBUG("masked = %s\n",zMap_g_list_quark_to_string(data->masked));
      if(data->masker) PDEBUG("masker = %s\n",zMap_g_list_quark_to_string(data->masker));
#endif
      for(fset = data->masked;fset;fset = fset->next)      // mask new featuresets by all
      {
            // with pipes we expect 1 featureset at a time but from ACE all 40 or so at once
            // so we do one ContextExecute for each one
            // which is OTT but we get thro' the aligns and blocks very quickly
            // and then the masker featuresets are found in the block's hash table
            // so really it's quite speedy

            data->mask_me = GPOINTER_TO_UINT(fset->data);
            zMapFeatureContextExecute((ZMapFeatureAny) view->features,
                                   ZMAPFEATURE_STRUCT_BLOCK,
                                   maskNewFeaturesetByAll,
                                   (gpointer) data);
      }


      if(data->masker)                                    // mask old featuresets by new data
      {
#if FILE_DEBUG
PDEBUG("%s","mask old by new\n");
#endif
            zMapFeatureContextExecute((ZMapFeatureAny) view->features,
                                   ZMAPFEATURE_STRUCT_FEATURESET,
                                   maskOldFeaturesetByNew,
                                   (gpointer) data);
      }
#if FILE_DEBUG
PDEBUG("%s","Completed\n");
#endif
      return(data->redisplay);
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
  ZMapFeatureSource src2src;
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
        ZMapFeatureSet feature_set = NULL;
        ZMapFeatureSet masker_set = NULL;

            /* do we have our target featureset in this block? */
        feature_set = (ZMapFeatureSet) g_hash_table_lookup(feature_any->children, GUINT_TO_POINTER(cb_data->mask_me));
        if(!feature_set)
            break;

        src2src = (ZMapFeatureSource) g_hash_table_lookup(cb_data->view->context_map.source_2_sourcedata, GUINT_TO_POINTER(feature_set->unique_id));
        if(!src2src)    // assert looks more natural but we get locus w/out any mapping from ACE
            break;

#if MH17_FEATURESET_HAS_NO_STYLE
        style =  zMapFindStyle(cb_data->view->context_map.styles,src2src->style_id);
#else
        style = feature_set->style;
#endif
        masked_by = zMapStyleGetMaskList(style);            /* all the masker featuresets */
#if FILE_DEBUG
PDEBUG("mask new by all: fset, style, masked by = %s, %s, %s\n", g_quark_to_string(feature_set->unique_id), g_quark_to_string(src2src->style_id), zMap_g_list_quark_to_string(masked_by));
#endif
        for(fset = masked_by;fset;fset = fset->next)
        {
            GQuark set_id = GPOINTER_TO_UINT(fset->data);

            if(set_id == g_quark_from_string("self"))
                  set_id = cb_data->mask_me;

            masker_set = (ZMapFeatureSet) g_hash_table_lookup(feature_any->children, GUINT_TO_POINTER(set_id));

            // has new data, must need sorting
            feature_set->masker_sorted_features = sortFeatureset(feature_set);


            if(masker_set)
                  mask_set_with_set(feature_set, masker_set);
        }
      }
      break;

    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;
        feature_set = (ZMapFeatureSet)feature_any;
      }
      /* fall through */
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

int twitter = 0;

/* order features by start coord then end coord reversed */
/* regardless of strand this still works */
gint fsetListOrderCB(gconstpointer a, gconstpointer b)
{
      GList *la = (GList *) a;
      GList *lb = (GList *) b;
      ZMapViewAlignSet sa = (ZMapViewAlignSet) la->data;
      ZMapViewAlignSet sb = (ZMapViewAlignSet) lb->data;

      if(sa->x1 < sb->x1)
            return(-1);
      if(sa->x1 > sb->x1)
            return(1);

      if(sa->x2 > sb->x2)
            return(-1);
      if(sa->x2 < sb->x2)
            return(1);
      return(0);
}


/* order feature by start coord */
gint fsetStartOrderCB(gconstpointer a, gconstpointer b)
{
      ZMapFeature fa = (ZMapFeature) a;
      ZMapFeature fb = (ZMapFeature) b;

      if(fa->x1 < fb->x1)
            return(-1);
      if(fa->x1 > fb->x1)
            return(1);
      return(0);
}



/* sort features in random name order using thier id quarks
 * we just want features with the same name to be together
 */
gint nameOrderCB(gconstpointer a, gconstpointer b)
{
      ZMapFeature fa = (ZMapFeature) a;
      ZMapFeature fb = (ZMapFeature) b;

      return ((gint) fa->original_id - (gint) fb->original_id);
}


/* related alignments have the same name but are distinct features
 * so we sort by name  and make lists of these
 * then we prepend an item to hold the start and end coord for the whole list
 *   using a noddy structure (can't get at the list end thanks to glib)
 * then we sort these into start coord then end coord reversed order
 */
static GList *sortFeatureset(ZMapFeatureSet fset)
{
      GList *l = NULL,*l_out = NULL;
      GList *gl_start, *gl_end;
      ZMapViewAlignSet align_set;
      ZMapFeature f_start,f_end,f;

      if(fset->masker_sorted_features)    /* free existing list of lists */
      {
            for(l = fset->masker_sorted_features;l;l = l->next)
            {
                  g_list_free((GList *) l->data);
            }
            g_list_free(fset->masker_sorted_features);
            fset->masker_sorted_features = NULL;
      }
      /* get pointers to all the features grouped according to name */
      zMap_g_hash_table_get_data(&l, fset->features);
      l = g_list_sort(l,nameOrderCB);

      /* chop this list into lists per name group
       * sorted by start coordinate
       * with a little header struct at the front
       * then add to another list,
       */

      for(gl_start = l;gl_start;)
      {
            f_start = (ZMapFeature) gl_start->data;

            for(gl_end = gl_start;gl_end;gl_end = gl_end->next)
            {
                  f_end = (ZMapFeature) gl_end->data;
                  if(f_end->original_id != f_start->original_id)
                        break;
            }

            if(gl_end)
            {
                  gl_end->prev->next = NULL;
                  gl_end->prev = NULL;
            }

            gl_start = g_list_sort(gl_start,fsetStartOrderCB);

            align_set = g_new0(ZMapViewAlignSetStruct,1);
            f = (ZMapFeature) gl_start->data;
            align_set->id = f->original_id;
            align_set->x1 = f->x1;

            l_out = g_list_prepend(l_out, g_list_prepend(gl_start,align_set));
            for(;gl_start;gl_start = gl_start->next)
            {
                  f = (ZMapFeature) gl_start->data;
                  align_set->x2 = f->x2;
            }

            gl_start = gl_end;
      }

      /* order these lists by start coord and end coord reversed */
      l_out = g_list_sort(l_out,fsetListOrderCB);
      return(l_out);
}

static gboolean maskOne(GList *f, GList *mask, gboolean exact)
{
      GList *l;
      ZMapFeature f_feat,m_feat;

      /* assuming strand data is not relevant and coordinates are always on fwd strand ...
       * see zmapFeature.h/ ZMapFeatureStruct.x1
       * now match each alignment in the list to the mask
       */
      for(l = mask;f;f = f->next)
      {
            f_feat = (ZMapFeature) f->data;
#if FILE_DEBUG
if(twitter) PDEBUG("fa %d-%d:",f_feat->x1,f_feat->x2);
#endif
            for(;l;l = l->next)
            {
                  m_feat = (ZMapFeature) l->data;
#if FILE_DEBUG
if(twitter) PDEBUG(" ma %d-%d",m_feat->x1,m_feat->x2);
#endif
                  if(m_feat->x1 <= f_feat->x1 && m_feat->x2 >= f_feat->x2)
                  {
                        /* matched this one */

                        if(exact)   /* must match splice junctions */
                        {
                              if(f->prev && m_feat->x1 != f_feat->x1)
                                    return(FALSE);
                              if(f->next && m_feat->x2 != f_feat->x2)
                                    return(FALSE);
                        }
                        break;
                  }

                  if(m_feat->x1 > f_feat->x1)
                        return(FALSE);    /* can't match this block so fail */

                  if(exact)
                        return(FALSE);    /* each block must match in turn */
            }
            if(!l)
                  return(FALSE);
            if(exact)                     /* set up for next block */
                  l = l->next;
#if FILE_DEBUG
if(twitter) PDEBUG("%s","\n");
#endif
      }
      return(TRUE);
}

static void mask_set_with_set(ZMapFeatureSet masked, ZMapFeatureSet masker)
{
      GList *ESTset,*mRNAset;
      GList *EST,*mRNA;
      ZMapViewAlignSet est,mrna;
      GList *m;
      gboolean exact = FALSE;

      int n_masked = 0;
      int n_failed = 0;
      int n_tried = 0;


      if(masked == masker)
      {
            /* each exon must correspond, no alt-splicing allowed
             * we sort into biggest first order (start then end coord reversed)
             * so masking against self should work easily
             */
            exact = TRUE;
      }

#if FILE_DEBUG
PDEBUG("mask set %s with set %s\n",
            g_quark_to_string(masked->original_id),
            g_quark_to_string(masker->original_id));
#endif

      /* for clarity we pretend we are masking an EST with an mRNA
       * but it could be EST x EST ro mRNA x mRNA
       */
      if(!masked->masker_sorted_features)
            masked->masker_sorted_features = sortFeatureset(masked);
      ESTset = masked->masker_sorted_features;

      if(!masker->masker_sorted_features)
            masker->masker_sorted_features = sortFeatureset(masker);
      mRNAset = masker->masker_sorted_features;

            /* is an EST completely covered by an mRNA?? */
      for(;ESTset;ESTset = ESTset->next)
      {
            n_tried++;

            EST = (GList *) ESTset->data;
            est = (ZMapViewAlignSet) EST->data;

            while(mRNAset)
            {
                  mRNA = (GList *) mRNAset->data;
                  mrna = (ZMapViewAlignSet) mRNA->data;
                  if(mrna->x2 > est->x1) // && mrna->x2 >= est->x2)
                        break;
                  mRNAset = mRNAset->next;
            }
            if(!mRNAset)                        /* no more masking possible */
                  break;

            if(est->x1 < mrna->x1)              /* is not covered */
                  continue;


            /* now the mRNA starts at or before the EST and ends after the EST starts */
            for(m = mRNAset;m && mrna->x1 <= est->x1;m = m->next)
            {
                  mRNA = (GList *) m->data;
                  mrna = (ZMapViewAlignSet) mRNA->data;

                  if(mrna == est)                     /* matching feature against self */
                        continue;

#if FILE_DEBUG
if(twitter) PDEBUG("EST %s = %d-%d (%d), mRNA = %s %d-%d (%d)\n",
      g_quark_to_string(est->id), est->x1,est->x2,g_list_length(EST) - 1,
      g_quark_to_string(mrna->id),mrna->x1,mrna->x2, g_list_length(mRNA) - 1);
#endif
                  if(!mrna->masked && !est->masked && mrna->x2 >= est->x2)
                  {
                        if(maskOne(EST->next,mRNA->next,exact))
                        {     /* this EST is covered by this mRNA */
                              GList *l;
                              ZMapFeature f;
                              n_masked++;
#if FILE_DEBUG
if(twitter) PDEBUG("%s"," masked\n");
#endif
                              for(l = EST->next;l;l = l->next)
                              {
                                    f = (ZMapFeature) l->data;
                                    f->feature.homol.flags.masked = TRUE;

                                    /* cannot un-display here as we have no windows */
                                    /* must post-process from the view */
                              }

                              /* ideally we'd like to remove this EST from the list
                               * but as we can have two copies of this list active (mask self)
                               * it would be a risky procedure
                               */
                              est->masked = TRUE;
                              break;
                        }
                        else
                        {
                              n_failed++;
#if FILE_DEBUG
if(twitter) PDEBUG("%s"," failed\n");
#endif
                        }
                  }

                  /* for exact (mask against self) it would be nice to break here
                   * but we have to consider alternate splicing
                   * biggest first is just the range not what's in the middle
                   */
            }
      }
#if FILE_DEBUG
PDEBUG("masked %d, failed %d, tried %d of %d composite features\n", n_masked,n_failed,n_tried,g_list_length(masked->masker_sorted_features));
#endif
}


/* mask old featureset with new data in same block */
/*NOTE for a self maskling featureset we do not process, as it's done by maskNewFeaturesetByAll() */
static ZMapFeatureContextExecuteStatus maskOldFeaturesetByNew(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapMaskFeatureSetData cb_data = (ZMapMaskFeatureSetData) user_data;
  GList *fset;
  GList *masked_by = NULL;
  ZMapFeatureSource src2src;
  ZMapFeatureTypeStyle style = NULL;

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

            /* this has to work or else this featureset could not be in the list */
        src2src = (ZMapFeatureSource) g_hash_table_lookup(cb_data->view->context_map.source_2_sourcedata,
                        GUINT_TO_POINTER(feature_any->unique_id));
        if(!(src2src))
        {
            /* could be a featuresset invented by ace and not having a src2src mapping
             * in which case we don't want to mask it anyway
             */
            break;
        }

#if MH17_FEATURESET_HAS_NO_STYLE
        style =  zMapFindStyle(cb_data->view->context_map.styles,src2src->style_id);
#else
        style = feature_set->style;
#endif

        if(style)
            masked_by = zMapStyleGetMaskList(style);        // all the maskers for this featuresets
#if FILE_DEBUG
//PDEBUG("mask old %s: %x %x\n",g_quark_to_string(feature_set->unique_id),(guint) style,(guint) masked_by);
#endif
        for(fset = masked_by;fset;fset = fset->next)
        {
            /* with pipes we expect one featureset at a time in which case this is trivial
             * with 40 featuresets from ACE we could have a large worst case
             * but realistically we expect only 2-3 masker featuresets
             * so it's not a performance problem despite the naive code
             */
            GQuark set_id = GPOINTER_TO_UINT(fset->data);
            GList *masker;

            if(set_id == g_quark_from_string("self"))
                  continue;   /* self maskers done by maskNewFeaturesetByAll() */

            masker = g_list_find(cb_data->masker,GUINT_TO_POINTER(set_id));

            if(masker)                                      /* only mask if new masker */
            {
                  GList *new;
#if FILE_DEBUG
PDEBUG("mask old %s with new %s\n",g_quark_to_string(feature_set->unique_id),g_quark_to_string(set_id));
#endif

                  new = g_list_find(cb_data->masked,GUINT_TO_POINTER(feature_set->unique_id));
                  if(!new)                                  /* only mask if featureset is old */
                  {
#if FILE_DEBUG
//PDEBUG("%s","is old\n");
#endif
                        masker_set = (ZMapFeatureSet)
                               g_hash_table_lookup(cb_data->block->feature_sets,masker->data);

                        if(masker_set)
                        {
#if FILE_DEBUG
//PDEBUG("%s","has masker in block\n");
#endif
                              mask_set_with_set(feature_set,masker_set);
                              cb_data->redisplay = g_list_prepend(cb_data->redisplay,
                                    GUINT_TO_POINTER(feature_set->unique_id));
                        }
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

