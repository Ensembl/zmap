/*  File: zmapFeatureContext.c
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions that operate on the feature context such as
 *              for reverse complementing.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapDNA.hpp>
#include <zmapFeature_P.hpp>




typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;

typedef struct _GetFeatureCBDataStruct
{
  GQuark set_id;
  ZMapFeature feature;
} GetFeatureCBDataStruct, *GetFeatureCBData;


typedef struct
{
  ZMapFeatureContext view_context; /* The Context to compare against */
  ZMapFeatureContext iteration_context; /* The Context we're stepping through comparing to the view */
  ZMapFeatureContext diff_context; /* The result of the comparison */

  /* The current path through the view context that we're currently on */
  ZMapFeatureAny current_view_align;
  ZMapFeatureAny current_view_block;
  ZMapFeatureAny current_view_set;

  /* The path through the diff context we're currently on. */
  ZMapFeatureAny current_diff_align;
  ZMapFeatureAny current_diff_block;
  ZMapFeatureAny current_diff_set;

  /* Our status... */
  ZMapFeatureContextExecuteStatus status;

  /* the list of real featuresets (not columns) actually requested */
  GList *req_featuresets;

  gboolean new_features ;/* this is a flag that includes featuresets and features */
  /* don't know if it matters if we flag featuresets */
  int feature_count;/* this is a count of the new features */

} MergeContextDataStruct, *MergeContextData;







#define ZMAP_CONTEXT_EXEC_NON_ERROR (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE | ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND)
#define ZMAP_CONTEXT_EXEC_RECURSE_OK (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE)
#define ZMAP_CONTEXT_EXEC_RECURSE_FAIL (~ ZMAP_CONTEXT_EXEC_RECURSE_OK)


typedef struct
{
  ZMapGDataRecurseFunc  start_callback;
  ZMapGDataRecurseFunc  end_callback;
  gpointer              callback_data;
  char                 *error_string;
  ZMapFeatureLevelType stop;
  ZMapFeatureLevelType stopped_at;
  unsigned int          use_remove : 1;
  unsigned int          use_steal  : 1;
  unsigned int          catch_hash : 1;
  ZMapFeatureContextExecuteStatus status;
}ContextExecuteStruct, *ContextExecute;



typedef struct
{
  int block_start, block_end ;
  int start;
  int end ;
  ZMapFeatureSet translation_fs ;
} RevCompDataStruct, *RevCompData ;



static void revCompFeature(ZMapFeature feature, int start_coord, int end_coord);
static ZMapFeatureContextExecuteStatus revCompFeaturesCB(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out);
static ZMapFeatureContextExecuteStatus revCompORFFeaturesCB(GQuark key,
                                                            gpointer data,
                                                            gpointer user_data,
                                                            char **error_out);
static void revcompSpan(GArray *spans, int start_coord, int seq_end) ;
static gboolean executeDataForeachFunc(gpointer key, gpointer data, gpointer user_data);

static ZMapFeatureContextExecuteStatus mergePreCB(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out) ;
static ZMapFeatureContextExecuteStatus eraseContextCB(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **err_out) ;

static ZMapFeatureContextExecuteStatus addEmptySets(GQuark key,
                                                    gpointer data,
                                                    gpointer user_data,
                                                    char **err_out);

static void mergeFeatureSetLoaded(ZMapFeatureSet view_set, ZMapFeatureSet new_set) ;
static ZMapFeatureContextExecuteStatus destroyIfEmptyContextCB(GQuark key,
                                                               gpointer data,
                                                               gpointer user_data,
                                                               char **err_out);


static void postExecuteProcess(ContextExecute execute_data);
static void copyQuarkCB(gpointer data, gpointer user_data) ;

static void feature_context_execute_full(ZMapFeatureAny feature_any,
                                         ZMapFeatureLevelType stop,
                                         ZMapGDataRecurseFunc start_callback,
                                         ZMapGDataRecurseFunc end_callback,
                                         gboolean use_remove,
                                         gboolean use_steal,
                                         gboolean catch_hash,
                                         gpointer data) ;

static ZMapFeatureContextExecuteStatus getFeaturesetFromIdCB(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **err_out) ;
static ZMapFeatureContextExecuteStatus getFeatureFromIdCB(GQuark key,
                                                          gpointer data,
                                                          gpointer user_data,
                                                          char **err_out) ;



/*
 *                    Globals
 */

static gboolean catch_hash_abuse_G = TRUE ;
static gboolean merge_debug_G = FALSE ;



/*
 *                    External routines
 */


ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end, GList *set_names)
{
  ZMapFeatureContext feature_context ;
  GQuark original_id = 0, unique_id = 0 ;

  if (sequence && *sequence)
    unique_id = original_id = g_quark_from_string(sequence) ;

  feature_context = (ZMapFeatureContext)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_CONTEXT,
                                                                    NULL,
                                                                    original_id, unique_id,
                                                                    NULL) ;

  if (sequence && *sequence)
    {
      feature_context->sequence_name = g_quark_from_string(sequence) ;
      feature_context->parent_span.x1 = start ;
      feature_context->parent_span.x2 = end ;
    }

  feature_context->req_feature_set_names = set_names ;

  return feature_context ;
}

gboolean zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
                                        ZMapFeatureAlignment alignment, gboolean master)
{
  gboolean result = FALSE  ;

  if (!feature_context || !alignment)
    return result ;

  if ((result = zmapFeatureAnyAddFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)alignment)))
    {
      if (master)
        feature_context->master_align = alignment ;
    }

  return result ;
}

gboolean zMapFeatureContextFindAlignment(ZMapFeatureContext   feature_context,
                                         ZMapFeatureAlignment feature_align)
{
  gboolean result = FALSE ;

  if (!feature_context || !feature_align )
    return result ;

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)feature_align) ;

  return result;
}

ZMapFeatureAlignment zMapFeatureContextGetAlignmentByID(ZMapFeatureContext feature_context,
                                                        GQuark align_id)
{
  ZMapFeatureAlignment feature_align ;

  feature_align = (ZMapFeatureAlignment)zMapFeatureParentGetFeatureByID((ZMapFeatureAny)feature_context, align_id) ;

  return feature_align ;
}


gboolean zMapFeatureContextRemoveAlignment(ZMapFeatureContext feature_context,
                                           ZMapFeatureAlignment feature_alignment)
{
  gboolean result = FALSE;

  if (!feature_context || !feature_alignment)
    return result ;

  if ((result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_context, (ZMapFeatureAny)feature_alignment)))
    {
      if(feature_context->master_align == feature_alignment)
        feature_context->master_align = NULL;
    }

  return result;
}



/* Context merging is complicated when it comes to destroying the contexts
 * because the diff_context actually points to feature structs in the merged
 * context. It has to do this because we want to pass to the drawing code
 * only new features that need drawing. To do this though means that some of
 * of the "parent" parts of the context (aligns, blocks, sets) must be duplicates
 * of those in the current context. Hence we end up with a context where we
 * want to destroy some features (the duplicates) but not others (the ones that
 * are just pointers to features in the current context).
 *
 * So for the diff_context we don't set destroy functions when the context
 * is created, instead we keep a separate hash of duplicate features to be destroyed.
 *
 * If hashtables supported setting a destroy function for each element we
 * wouldn't need to do this, but they don't (unlike g_datalists, we don't
 * use those because they are too slow).
 *
 * If all ok returns ZMAPFEATURE_CONTEXT_OK, merged context in merged_context_inout
 * and diff context in diff_context_out. Otherwise returns a code to show what went
 * wrong, unaltered original context in merged_context_inout and diff_context_out is NULL.
 *
 * N.B. under new scheme, new_context_inout will be always be destroyed && NULL'd....
 *
 *
 * If merge_stats_out is non-NULL then a pointer to a struct containing stats about the merge is
 * returned in merge_stats_out, the struct should be g_free'd when no longer required.
 *
 */
ZMapFeatureContextMergeCode zMapFeatureContextMerge(ZMapFeatureContext *merged_context_inout,
                                                    ZMapFeatureContext *new_context_inout,
                                                    ZMapFeatureContext *diff_context_out,
                                                    ZMapFeatureContextMergeStats *merge_stats_out,
                                                    GList *featureset_names)
{
  ZMapFeatureContextMergeCode status = ZMAPFEATURE_CONTEXT_ERROR ;
  ZMapFeatureContext current_context, new_context, diff_context = NULL ;
  MergeContextDataStruct merge_data = {NULL} ;

  if (!merged_context_inout || !new_context_inout || !diff_context_out)
    return status ;

  current_context = *merged_context_inout ;
  new_context = *new_context_inout ;


  /* If there are no current features we just return the new one as the merged and the diff,
   * otherwise we need to do a merge of the new and current feature sets. */

  /* the view is now initialised with an empty context to aoid
     race conditions  between Rx featuresets and display
  */

  /* if several servers supply data before the first gets painted then some get painted twice
   * (data is duplicated in the canvas) */
  if (!current_context)
    {
      if (merge_debug_G)
        zMapLogWarning("%s", "No current context, returning complete new...") ;

      diff_context = current_context = new_context ;
      new_context = NULL ;

      merge_data.view_context      = current_context;
      merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK ;
      merge_data.req_featuresets   = featureset_names;

      zMapFeatureContextExecute((ZMapFeatureAny) current_context,
                                ZMAPFEATURE_STRUCT_BLOCK, addEmptySets, &merge_data);

      status = ZMAPFEATURE_CONTEXT_OK ;
    }
  else
    {
      /* Here we need to merge for all alignments and all blocks.... */
      GList *copy_features ;


      /* Note we make the diff context point at the feature list and styles of the new context,
       * I guess we could copy them but doesn't seem worth it...see code below where we NULL them
       * in new_context so they are not thrown away.... */
      diff_context = (ZMapFeatureContext)zmapFeatureAnyCopy((ZMapFeatureAny)new_context, NULL) ;
      diff_context->diff_context = TRUE ;
      diff_context->elements_to_destroy = g_hash_table_new_full(NULL, NULL, NULL, zmapDestroyFeatureAny) ;
      diff_context->req_feature_set_names = g_list_copy(new_context->req_feature_set_names) ;
      diff_context->src_feature_set_names = NULL;

      merge_data.view_context      = current_context;
      merge_data.iteration_context = new_context;
      merge_data.diff_context      = diff_context ;
      merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK ;
      merge_data.new_features = FALSE ;
      merge_data.feature_count = 0;
      merge_data.req_featuresets   = new_context->req_feature_set_names;


      /* THIS LOOKS SUSPECT...WHY ISN'T THE NAMES LIST COPIED FROM NEW_CONTEXT....*/
      copy_features = g_list_copy(new_context->req_feature_set_names) ;
      current_context->req_feature_set_names = g_list_concat(current_context->req_feature_set_names,
                                                             copy_features) ;

      if (merge_debug_G)
        zMapLogWarning("%s", "merging ...");

      /* Do the merge ! */
      zMapFeatureContextExecuteStealSafe((ZMapFeatureAny)new_context, ZMAPFEATURE_STRUCT_FEATURE,
                                         mergePreCB, NULL, &merge_data) ;

      if (merge_debug_G)
        zMapLogWarning("%s", "finished ...");


      if (merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
        {
          /* Set these to NULL as diff_context references them. */
          new_context->req_feature_set_names = NULL ;

          current_context = merge_data.view_context ;
          new_context     = merge_data.iteration_context ;
          diff_context    = merge_data.diff_context ;

          if (merge_data.new_features)
            {


              if (merge_debug_G)
                {
                  /* Debug stuff... */
                  GError *err = NULL ;

                  printf("(Merge) diff context:\n") ;
                  zMapFeatureDumpStdOutFeatures(diff_context, NULL, &err) ;
                }


              if (merge_debug_G)
                {
                  /* Debug stuff... */
                  GError *err = NULL ;

                  printf("(Merge) full context:\n") ;
                  zMapFeatureDumpStdOutFeatures(current_context, NULL, &err) ;
                }



#ifdef MH17_NEVER
              // NB causes crash on 2nd load, first one is ok
              {
                GError *err = NULL;

                zMapFeatureDumpToFileName(diff_context,"features.txt","(Merge) diff context:\n", NULL, &err) ;
                zMapFeatureDumpToFileName(current_context,"features.txt","(Merge) full context:\n", NULL, &err) ;
              }
#endif

              status = ZMAPFEATURE_CONTEXT_OK ;
            }
          else
            {
              if (diff_context)
                {
                  zMapFeatureContextDestroy(diff_context, TRUE);

                  diff_context = NULL ;
                }

              status = ZMAPFEATURE_CONTEXT_NONE ;
            }
        }
    }

  /* Clear up new_context whatever happens. If there is an error, what can anyone do with it...nothing. */
  if (new_context)
    {
      zMapFeatureContextDestroy(new_context, TRUE);
      new_context = NULL ;
    }

  if (status == ZMAPFEATURE_CONTEXT_OK || status == ZMAPFEATURE_CONTEXT_NONE)
    {
      *merged_context_inout = current_context ;
      *new_context_inout = new_context ;
      *diff_context_out = diff_context ;
    }

  if (merge_stats_out)
    {
      ZMapFeatureContextMergeStats merge_stats ;

      merge_stats = g_new0(ZMapFeatureContextMergeStatsStruct, 1) ;
      merge_stats->features_added = merge_data.feature_count ;

      *merge_stats_out = merge_stats ;
    }

  return status ;
}



/* creates a context of features of matches between the remove and
 * current, removing from the current as it goes. */
gboolean zMapFeatureContextErase(ZMapFeatureContext *current_context_inout,
                                 ZMapFeatureContext remove_context,
                                 ZMapFeatureContext *diff_context_out)
{
  gboolean erased = FALSE;
  /* Here we need to merge for all alignments and all blocks.... */
  MergeContextDataStruct merge_data = {NULL};
  char *diff_context_string  = NULL;
  ZMapFeatureContext current_context ;
  ZMapFeatureContext diff_context ;
  GList *copy_list ;

  if (!current_context_inout || !remove_context || !diff_context_out)
    return erased ;

  current_context = *current_context_inout ;

  diff_context = zMapFeatureContextCreate(NULL, 0, 0, NULL);
  diff_context->diff_context        = TRUE;
  diff_context->elements_to_destroy = g_hash_table_new_full(NULL, NULL, NULL, zmapDestroyFeatureAny);

  merge_data.view_context      = current_context;
  merge_data.iteration_context = remove_context;
  merge_data.diff_context      = diff_context;
  merge_data.status            = ZMAP_CONTEXT_EXEC_STATUS_OK;


  /* LOOKS SUSPECT...SHOULD BE COPIED....*/
  diff_context->req_feature_set_names = g_list_copy(remove_context->req_feature_set_names) ;

  copy_list = g_list_copy(remove_context->req_feature_set_names) ;
  current_context->req_feature_set_names = g_list_concat(current_context->req_feature_set_names,
                                                         copy_list) ;

  /* Set the original and unique ids so that the context passes the feature validity checks */
  diff_context_string = g_strdup_printf("%s vs %s\n",
                                        g_quark_to_string(current_context->unique_id),
                                        g_quark_to_string(remove_context->unique_id));
  diff_context->original_id =
    diff_context->unique_id = g_quark_from_string(diff_context_string);

  g_free(diff_context_string);

  zMapFeatureContextExecuteRemoveSafe((ZMapFeatureAny)remove_context, ZMAPFEATURE_STRUCT_FEATURE,
                                      eraseContextCB, destroyIfEmptyContextCB, &merge_data);


  if(merge_data.status == ZMAP_CONTEXT_EXEC_STATUS_OK)
    {
      erased = TRUE;

      *current_context_inout = current_context ;
      *diff_context_out      = diff_context ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if(merge_erase_dump_context_G)
        {
          GError *err = NULL;

          printf("(Erase) diff context:\n") ;
          zMapFeatureDumpStdOutFeatures(diff_context, diff_context->styles, &err) ;

          printf("(Erase) full context:\n") ;
          zMapFeatureDumpStdOutFeatures(current_context, current_context->styles, &err) ;

        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return erased;
}


void zMapFeatureContextDestroy(ZMapFeatureContext feature_context, gboolean free_data)
{
  if (!feature_context || !zMapFeatureIsValid((ZMapFeatureAny)feature_context))
    return ;

  if (feature_context->diff_context)
    {
      zmapDestroyFeatureAny(feature_context) ;
    }
  else
    {
      zmapDestroyFeatureAnyWithChildren((ZMapFeatureAny)feature_context, free_data) ;
    }

  return ;
}



/* Reverse complement a feature context.
 *
 * Efficiency does not allow us to simply throw everything away and reconstruct the context.
 * Therefore we have to go through and carefully reverse everything.
 *
 * features need to have their positions reversed and also their strand.
 * what about blocks ??? they also need doing... and in fact there are the alignment
 * mappings etc.....needs some thought and effort....
 *
 */
void zMapFeatureContextReverseComplement(ZMapFeatureContext context)
{
  RevCompDataStruct cb_data ;

  cb_data.start = context->parent_span.x1;
  cb_data.end   = context->parent_span.x2 ;

  cb_data.block_start = 0 ;
  cb_data.block_end = 0 ;
  cb_data.translation_fs = NULL ;

  //zMapLogWarning("rev comp, parent span = %d -> %d",context->parent_span.x1,context->parent_span.x2);

  /* Because this doesn't allow for execution at context level ;( */
  zMapFeatureContextExecute((ZMapFeatureAny)context,
                            ZMAPFEATURE_STRUCT_FEATURE,
                            revCompFeaturesCB,
                            &cb_data);


  //GQuark featureset_id = g_quark_from_string(ZMAP_FIXED_STYLE_ORF_NAME);
  //zMapFeatureAnyGetFeatureByID(context, featureset_id) ;

  zMapFeatureContextExecute((ZMapFeatureAny)context,
                            ZMAPFEATURE_STRUCT_FEATURE,
                            revCompORFFeaturesCB,
                            &cb_data);

  return ;
}



/* this is for a block only ! */
void zMapFeatureReverseComplement(ZMapFeatureContext context, ZMapFeature feature)
{
  int start, end ;

  start = context->parent_span.x1 ;
  end   = context->parent_span.x2 ;

  revCompFeature(feature, start, end) ;

  return ;
}

void zMapFeatureReverseComplementCoords(ZMapFeatureContext context, int *start_inout, int *end_inout)
{
  int start, end, my_start, my_end ;

  start = context->parent_span.x1 ;
  end   = context->parent_span.x2 ;

  my_start = *start_inout ;
  my_end = *end_inout ;

  zmapFeatureRevComp(start, end, &my_start, &my_end) ;

  *start_inout = my_start ;
  *end_inout = my_end ;

  return ;
}


/* Take a string containing space separated context names (e.g. perhaps a list
 * of featuresets: "coding fgenes codon") and convert it to a list of
 * proper context id quarks. */
GList *zMapFeatureCopyQuarkList(GList *quark_list_orig)
{
  GList *quark_list_new = NULL ;

  g_list_foreach(quark_list_orig, copyQuarkCB, &quark_list_new) ;

  return quark_list_new ;
}


/* This function could be implemented using the contextexecute functions but that would
 * be quite wasteful and inefficient as they go down the hierachy and we only need to go
 * up. */
ZMapFeatureContext zMapFeatureContextCopyWithParents(ZMapFeatureAny orig_feature)
{
  ZMapFeatureContext copied_context = NULL ;

  if (zMapFeatureIsValid(orig_feature))
    {
      ZMapFeatureAny curr = orig_feature, curr_copy = NULL, prev_copy = NULL ;

      do
        {
          curr_copy = zMapFeatureAnyCopy(curr) ;

          switch(curr->struct_type)
            {
            case ZMAPFEATURE_STRUCT_FEATURE:
              {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                ZMapFeature feature = (ZMapFeature)curr_copy ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


                break;
              }

            case ZMAPFEATURE_STRUCT_FEATURESET:
              {
                ZMapFeatureSet feature_set = (ZMapFeatureSet)curr_copy ;
                ZMapFeature feature = (ZMapFeature)prev_copy ;

                if (feature)
                  zMapFeatureSetAddFeature(feature_set, feature) ;

                break;
              }

            case ZMAPFEATURE_STRUCT_BLOCK:
              {
                ZMapFeatureBlock block = (ZMapFeatureBlock)curr_copy ;
                ZMapFeatureSet feature_set = (ZMapFeatureSet)prev_copy ;

                if (feature_set)
                  zMapFeatureBlockAddFeatureSet(block, feature_set) ;

                break;
              }

            case ZMAPFEATURE_STRUCT_ALIGN:
              {
                ZMapFeatureAlignment align = (ZMapFeatureAlignment)curr_copy ;
                ZMapFeatureBlock block = (ZMapFeatureBlock)prev_copy ;

                if (block)
                  zMapFeatureAlignmentAddBlock(align, block) ;

                break;
              }

            case ZMAPFEATURE_STRUCT_CONTEXT:
              {
                ZMapFeatureContext context = (ZMapFeatureContext)curr_copy ;
                ZMapFeatureAlignment align = (ZMapFeatureAlignment)prev_copy ;

                if (align)
                  zMapFeatureContextAddAlignment(context, align, TRUE) ;

                copied_context = context ;

                break;
              }

            default:
              zMapWarnIfReached();
              break;
            }

          if (prev_copy)
            {
              prev_copy->parent = curr_copy ;
            }


          prev_copy = curr_copy ;
          curr = curr->parent ;

        } while (curr) ;


    }


  return copied_context ;
}




/*!
 * \brief A similar call to g_datalist_foreach. However there are a number of differences.
 *
 * - 1 - There's more than one GData * involved
 * - 2 - There's more than just GData's involved (Blocks are still GLists) (No longer true)
 * - 3 - Because there is a hierarchy you can specify a stop level
 * - 4 - You get multiple Feature types in your GDataForeachFunc, which you have to handle.
 *
 * N.B. The First level in the callback is the alignment _not_ context, to get the context
 *      too use zMapFeatureContextExecuteFull
 *
 * @param feature_any Provide absolutely any level of feature and it will start from the top
 *                    for you.  i.e. find the context and work it's way back down through
 *                    the tree until...
 * @param stop        The level at which you want to descend _no_ further.  callback *will*
 *                    be called at this level, but that's it.
 * @param callback    The GDataForeachFunc you want to be called on each FEATURE.  This
 *                    _includes_ blocks.  There is a helper function which will just call
 *                    your function with the block->unique_id as the key as if they really
 *                    were in a GData.
 * @param data        Data of your own type for your callback...
 * @return void.
 *
 */

void zMapFeatureContextExecute(ZMapFeatureAny feature_any,
                               ZMapFeatureLevelType stop,
                               ZMapGDataRecurseFunc callback,
                               gpointer data)
{
  ContextExecuteStruct full_data = {0};
  ZMapFeatureContext context = NULL;

  if(stop != ZMAPFEATURE_STRUCT_INVALID)
    {
      context = (ZMapFeatureContext)zMapFeatureGetParentGroup(feature_any, ZMAPFEATURE_STRUCT_CONTEXT);
      full_data.start_callback = callback;
      full_data.end_callback   = NULL;
      full_data.callback_data  = data;
      full_data.stop           = stop;
      full_data.stopped_at     = ZMAPFEATURE_STRUCT_INVALID;
      full_data.status         = ZMAP_CONTEXT_EXEC_STATUS_OK;
      full_data.use_remove     = FALSE;
      full_data.use_steal      = FALSE;
      full_data.catch_hash     = catch_hash_abuse_G;

      /* Start it all off with the alignments */
      if(full_data.use_remove)
        g_hash_table_foreach_remove(context->alignments, executeDataForeachFunc, &full_data) ;
      else if (context)
        g_hash_table_foreach(context->alignments, (GHFunc)executeDataForeachFunc, &full_data) ;

      postExecuteProcess(&full_data);
    }

  return ;
}

void zMapFeatureContextExecuteSubset(ZMapFeatureAny feature_any,
                                     ZMapFeatureLevelType stop,
                                     ZMapGDataRecurseFunc callback,
                                     gpointer data)
{

  if(stop != ZMAPFEATURE_STRUCT_INVALID)
    {
      ContextExecuteStruct full_data = {0};
      full_data.start_callback = callback;
      full_data.end_callback   = NULL;
      full_data.callback_data  = data;
      full_data.stop           = stop;
      full_data.stopped_at     = ZMAPFEATURE_STRUCT_INVALID;
      full_data.status         = ZMAP_CONTEXT_EXEC_STATUS_OK;
      full_data.use_remove     = FALSE;
      full_data.use_steal      = FALSE;
      full_data.catch_hash     = catch_hash_abuse_G;

      if(feature_any->struct_type <= stop)
        executeDataForeachFunc(GINT_TO_POINTER(feature_any->unique_id), feature_any, &full_data);
      else
        full_data.error_string = g_strdup("Too far down the hierarchy...");

      postExecuteProcess(&full_data);
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

void zMapFeatureContextExecuteFull(ZMapFeatureAny feature_any,
                                   ZMapFeatureLevelType stop,
                                   ZMapGDataRecurseFunc callback,
                                   gpointer data)
{
  feature_context_execute_full(feature_any, stop, callback,
                               NULL, FALSE, FALSE,
                               catch_hash_abuse_G, data);
  return ;
}

void zMapFeatureContextExecuteComplete(ZMapFeatureAny feature_any,
                                       ZMapFeatureLevelType stop,
                                       ZMapGDataRecurseFunc start_callback,
                                       ZMapGDataRecurseFunc end_callback,
                                       gpointer data)
{
  feature_context_execute_full(feature_any, stop, start_callback,
                               end_callback, FALSE, FALSE,
                               catch_hash_abuse_G, data);
  return ;
}

/* Use this when the feature_any tree gets modified during traversal */
void zMapFeatureContextExecuteRemoveSafe(ZMapFeatureAny feature_any,
                                         ZMapFeatureLevelType stop,
                                         ZMapGDataRecurseFunc start_callback,
                                         ZMapGDataRecurseFunc end_callback,
                                         gpointer data)
{
  feature_context_execute_full(feature_any, stop, start_callback,
                               end_callback, TRUE, FALSE,
                               catch_hash_abuse_G, data);
  return ;
}

void zMapFeatureContextExecuteStealSafe(ZMapFeatureAny feature_any, ZMapFeatureLevelType stop,
                                        ZMapGDataRecurseFunc start_callback, ZMapGDataRecurseFunc end_callback,
                                        gpointer data)
{
  feature_context_execute_full(feature_any, stop, start_callback,
                               end_callback, FALSE, TRUE,
                               catch_hash_abuse_G, data);
  return ;
}


/* This seems like an insane thing to ask for... Why would you want to
 * find a feature given a feature. Well the from feature will _not_ be
 * residing in the context, but must be in another context.
 */

ZMapFeatureAny zMapFeatureContextFindFeatureFromFeature(ZMapFeatureContext context, ZMapFeatureAny query_feature)
{
  ZMapFeatureAny
    feature_any   = NULL,
    feature_ptr   = NULL,
    query_align   = NULL,
    query_block   = NULL,
    query_set     = NULL;

  /* Go up in the query feature's tree to the top, recording on the way */
  feature_ptr = query_feature;
  while(feature_ptr && feature_ptr->struct_type > ZMAPFEATURE_STRUCT_CONTEXT)
    {
      feature_ptr = zMapFeatureGetParentGroup(feature_ptr, (ZMapFeatureLevelType)(feature_ptr->struct_type - 1));
      switch(feature_ptr->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          break;
        case ZMAPFEATURE_STRUCT_ALIGN:
          query_align   = feature_ptr;
          break;
        case ZMAPFEATURE_STRUCT_BLOCK:
          query_block   = feature_ptr;
          break;
        case ZMAPFEATURE_STRUCT_FEATURESET:
          query_set     = feature_ptr;
          break;
        case ZMAPFEATURE_STRUCT_FEATURE:
        default:
          zMapWarnIfReached();
          break;
        }
    }

  /* Now go back down in the target context */
  feature_ptr = (ZMapFeatureAny)context;
  while(feature_ptr && feature_ptr->struct_type < ZMAPFEATURE_STRUCT_FEATURE)
    {
      ZMapFeatureAny current = NULL;
      switch(feature_ptr->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          current = query_align;
          break;
        case ZMAPFEATURE_STRUCT_ALIGN:
          current = query_block;
          break;
        case ZMAPFEATURE_STRUCT_BLOCK:
          current = query_set;
          break;
        case ZMAPFEATURE_STRUCT_FEATURESET:
          current = query_feature;
          break;
        case ZMAPFEATURE_STRUCT_FEATURE:
        default:
          zMapWarnIfReached();
          break;
        }

      if(current)
        feature_ptr = (ZMapFeatureAny)g_hash_table_lookup(feature_ptr->children,
                                                          zmapFeature2HashKey(current));
      else
        feature_ptr = NULL;
    }

  if((feature_ptr) &&
     (feature_ptr != query_feature) &&
     (feature_ptr->struct_type == query_feature->struct_type))
    feature_any = feature_ptr;

  return feature_any;
}



/*
 *                  Package routines
 */
void zmapFeatureRevCompCoord(int *coord, const int start, const int end)
{
  zmapFeatureInvert(coord,start,end);
}


/*!
 * \brief Find the featureset with the given id in the given context
 */
ZMapFeatureSet zmapFeatureContextGetFeaturesetFromId(ZMapFeatureContext context, GQuark set_id)
{
  GetFeaturesetCBDataStruct cb_data = { set_id, NULL };

  zMapFeatureContextExecute((ZMapFeatureAny)context,
                            ZMAPFEATURE_STRUCT_FEATURESET,
                            getFeaturesetFromIdCB,
                            &cb_data);

  return cb_data.featureset ;
}

/*!
 * \brief Find the feature with the given id in the given context
 */
ZMapFeature zmapFeatureContextGetFeatureFromId(ZMapFeatureContext context, GQuark feature_id)
{
  GetFeatureCBDataStruct cb_data = { feature_id, NULL };

  zMapFeatureContextExecute((ZMapFeatureAny)context,
                            ZMAPFEATURE_STRUCT_FEATURE,
                            getFeatureFromIdCB,
                            &cb_data);

  return cb_data.feature ;
}





/*
 *                        Internal functions.
 */



/* A GFunc() to copy a list member that holds just a quark. */
static void copyQuarkCB(gpointer data, gpointer user_data)
{
  GQuark orig_id = GPOINTER_TO_INT(data) ;
  GList **new_list_ptr = (GList **)user_data ;
  GList *new_list = *new_list_ptr ;

  new_list = g_list_append(new_list, GINT_TO_POINTER(orig_id)) ;

  *new_list_ptr = new_list ;

  return ;
}

static ZMapFeatureContextExecuteStatus revCompFeaturesCB(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  RevCompData cb_data = (RevCompData)user_data;

  if (!feature_any || !zMapFeatureIsValid(feature_any))
    return status ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;

        /* we revcomp a complete context so reflect in the parent span */

        /* THIS LOOKS DEEPLY INCORRECT TO ME..... */

        zmapFeatureRevComp(cb_data->start, cb_data->end,
                           &feature_align->sequence_span.x1,
                           &feature_align->sequence_span.x2) ;

        break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;

        feature_block  = (ZMapFeatureBlock)feature_any;

        /* Before reversing blocks coords record what they are so we can use them for peptides
         * and other special case revcomps. */
        cb_data->block_start = feature_block->block_to_sequence.block.x1 ;
        cb_data->block_end = feature_block->block_to_sequence.block.x2 ;


        /* Complement the dna. */
        if (feature_block->sequence.sequence)
          {
            zMapDNAReverseComplement(feature_block->sequence.sequence, feature_block->sequence.length) ;
          }

        zmapFeatureRevComp(cb_data->start, cb_data->end,
                           &feature_block->block_to_sequence.block.x1,
                           &feature_block->block_to_sequence.block.x2) ;
        feature_block->revcomped = !feature_block->revcomped;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;
        GList *l;
        ZMapSpan span;

        feature_set = (ZMapFeatureSet)feature_any;

        /* need to rev comp the loaded regions list */
        for (l = feature_set->loaded;l;l = l->next)
          {
            span = (ZMapSpan) l->data;
            zmapFeatureRevComp(cb_data->start, cb_data->end, &span->x1, &span->x2) ;
          }


        /* OK...THIS IS CRAZY....SHOULD BE PART OF THE FEATURE REVCOMP....FIX THIS.... */
        /* Now redo the 3 frame translations from the dna (if they exist). */
        if (feature_set->unique_id == zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME))
          {
            cb_data->translation_fs = feature_set;
            zmapFeature3FrameTranslationSetRevComp(feature_set, cb_data->block_start, cb_data->block_end) ;
          }

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature_ft = NULL;

        feature_ft = (ZMapFeature)feature_any ;

        revCompFeature(feature_ft, cb_data->start, cb_data->end) ;

        break;
      }
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
        zMapWarnIfReached();
        break;
      }
    }

  return status;
}


static ZMapFeatureContextExecuteStatus revCompORFFeaturesCB(GQuark key,
                                                            gpointer data,
                                                            gpointer user_data,
                                                            char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  RevCompData cb_data = (RevCompData)user_data;

  if (!feature_any || !zMapFeatureIsValid(feature_any))
    return status ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;

        feature_set = (ZMapFeatureSet)feature_any;

        if (feature_set->original_id == g_quark_from_string(ZMAP_FIXED_STYLE_ORF_NAME))
          zmapFeatureORFSetRevComp(feature_set, cb_data->translation_fs);


        break;
      }
    default:
      {
        break;
      }
    }

  return status;
}


/* NOTE this is for transcript exon and intron arrays, also assembly paths (which are not used) */
/* we need the exons etc to be in fwd order so we have to reverse the array as well as revcomp'ing the coords */
static void revcompSpan(GArray *spans, int seq_start, int seq_end)
{
  int i, j;

  for (i = 0; i < (int)spans->len; i++)
    {
      ZMapSpan span ;

      span = &g_array_index(spans, ZMapSpanStruct, i) ;

      zmapFeatureRevComp(seq_start, seq_end, &span->x1, &span->x2) ;
    }

  for (i = 0, j = spans->len - 1 ; i < j ; i++,j--)
    {
      ZMapSpanStruct x ;
      ZMapSpan si,sj ;

      si = &g_array_index(spans, ZMapSpanStruct, i) ;
      sj = &g_array_index(spans, ZMapSpanStruct, j) ;

      x = *si ;    /* struct copies */
      *si = *sj ;
      *sj = x ;
    }

  return ;
}


static void revCompFeature(ZMapFeature feature, int start_coord, int end_coord)
{
  if (!feature)
    return ;

  zmapFeatureRevComp(start_coord, end_coord, &feature->x1, &feature->x2) ;

  if (feature->strand == ZMAPSTRAND_FORWARD)
    feature->strand = ZMAPSTRAND_REVERSE ;
  else if (feature->strand == ZMAPSTRAND_REVERSE)
    feature->strand = ZMAPSTRAND_FORWARD ;

  if (zMapFeatureSequenceIsPeptide(feature))
    {
      /* Original & Unique IDs need redoing as they include the frame which probably change on revcomp. */
      char *feature_name = NULL ;    /* Remember to free this */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      GQuark feature_id ;
#endif

      /* Essentially this does nothing and needs removing... */


      feature_name = zMapFeature3FrameTranslationFeatureName((ZMapFeatureSet)(feature->parent),
                                                             zMapFeatureFrame(feature)) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      feature_id = g_quark_from_string(feature_name) ;
#endif
      g_free(feature_name) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      feature->original_id = feature->unique_id = feature_id ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  else if (zMapFeatureSequenceIsDNA(feature))
    {
      /* Unique ID needs redoing as it includes the coords which change on revcomp. */
      ZMapFeatureBlock block ;
      GQuark dna_id;

      block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)(feature), ZMAPFEATURE_STRUCT_BLOCK)) ;

      dna_id = zMapFeatureDNAFeatureID(block) ;

      feature->unique_id = dna_id ;
    }
  else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT
           && (feature->feature.transcript.exons || feature->feature.transcript.introns))
    {
      if (feature->feature.transcript.exons)
        revcompSpan(feature->feature.transcript.exons, start_coord, end_coord) ;

      if (feature->feature.transcript.introns)
        revcompSpan(feature->feature.transcript.introns, start_coord, end_coord) ;

      if(feature->feature.transcript.flags.cds)
        zmapFeatureRevComp(start_coord, end_coord,
                           &feature->feature.transcript.cds_start,
                           &feature->feature.transcript.cds_end);
    }
  else if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT
           && feature->feature.homol.align)
    {
      int i ;

      for (i = 0; i < (int)feature->feature.homol.align->len; i++)
        {
          ZMapAlignBlock align ;

          align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;

          zmapFeatureRevComp(start_coord, end_coord, &align->t1, &align->t2) ;
          /*! \todo #warning why does this not change t_strand? */
        }

      zMapFeatureSortGaps(feature->feature.homol.align) ;

      if(feature->feature.homol.sequence && *feature->feature.homol.sequence)/* eg if provided in GFF (BAM) */
        {
          if (feature->feature.homol.length != (int)strlen(feature->feature.homol.sequence))
            printf("%s: seq lengths differ: %d, %zd\n",
                   g_quark_to_string(feature->original_id), feature->feature.homol.length,
                   strlen(feature->feature.homol.sequence));

          zMapDNAReverseComplement(feature->feature.homol.sequence, feature->feature.homol.length) ;
        }
    }
  else if (feature->mode == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
    {
      if (feature->feature.assembly_path.path)
        revcompSpan(feature->feature.assembly_path.path, start_coord, end_coord) ;
    }

  return ;
}


static ZMapFeatureContextExecuteStatus print_featureset_name(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;

  feature_type = feature_any->struct_type;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;
      printf("    featureset %s\n",g_quark_to_string(feature_set->unique_id));

    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
      status = ZMAP_CONTEXT_EXEC_STATUS_OK;
      break;
    default:
      break;
    }

  return status;
}

void zMapPrintContextFeaturesets(ZMapFeatureContext context)
{
  zMapFeatureContextExecute((ZMapFeatureAny) context,
                            ZMAPFEATURE_STRUCT_FEATURESET,
                            print_featureset_name, NULL);
}

#ifdef RDS_TEMPLATE_USER_DATALIST_FOREACH
/* Use this function while descending through a feature context */
static ZMapFeatureContextExecuteStatus templateDataListForeach(GQuark key,
                                                               gpointer data,
                                                               gpointer user_data,
                                                               char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContext feature_context = NULL;
  ZMapFeatureAlignment feature_align = NULL;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeature          feature_ft    = NULL;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;


  feature_type = feature_any->struct_type;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      feature_context = (ZMapFeatureContext)feature_any;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      feature_align = (ZMapFeatureAlignment)feature_any;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      feature_ft = (ZMapFeature)feature_any;
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapWarnIfReached();
      break;

    }

  return status;
}
#endif /* RDS_TEMPLATE_USER_DATALIST_FOREACH */

/* A GHRFunc() */
static gboolean  executeDataForeachFunc(gpointer key_ptr, gpointer data, gpointer user_data)
{
  GQuark key = GPOINTER_TO_INT(key_ptr) ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ContextExecute full_data = (ContextExecute)user_data;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  gboolean  remove_from_hash = FALSE;

  if (!zMapFeatureAnyHasMagic(feature_any))
    return remove_from_hash ;

  if(full_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK ||
     full_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE)
    {
      feature_type = feature_any->struct_type;

      if(full_data->start_callback)
        full_data->status = (full_data->start_callback)(key, data,
                                                        full_data->callback_data,
                                                        &(full_data->error_string));

      /* If the callback returns DELETE then should we still descend?
       * Yes. In order to stop descending it should return DONT_DESCEND */

      /* use one's complement of ZMAP_CONTEXT_EXEC_RECURSE_OK to check
       * status does not now have any bits not in
       * ZMAP_CONTEXT_EXEC_RECURSE_OK */
      if(((full_data->stop == feature_type) == FALSE) &&
         (!(full_data->status & ZMAP_CONTEXT_EXEC_RECURSE_FAIL)))
        {
          switch(feature_type)
            {
            case ZMAPFEATURE_STRUCT_CONTEXT:
            case ZMAPFEATURE_STRUCT_ALIGN:
            case ZMAPFEATURE_STRUCT_BLOCK:
            case ZMAPFEATURE_STRUCT_FEATURESET:
              {
                int child_count_before, children_removed, child_count_after;

                if(full_data->catch_hash)
                  {
                    child_count_before = g_hash_table_size(feature_any->children);
                    children_removed   = 0;
                    child_count_after  = 0;
                  }

                if(full_data->use_remove)
                  children_removed = g_hash_table_foreach_remove(feature_any->children,
                                                                 executeDataForeachFunc,
                                                                 full_data) ;
                else if(full_data->use_steal)
                  children_removed = g_hash_table_foreach_steal(feature_any->children,
                                                                executeDataForeachFunc,
                                                                full_data) ;
                else
                  g_hash_table_foreach(feature_any->children,
                                       (GHFunc)executeDataForeachFunc,
                                       full_data) ;

                if(full_data->catch_hash)
                  {
                    child_count_after = g_hash_table_size(feature_any->children);

                    if((child_count_after + children_removed) != child_count_before)
                      {
                        zMapLogWarning("%s", "Altering hash during foreach _not_ supported!");
                        zMapLogCritical("Hash traversal on the children of '%s' isn't going to work",
                                        g_quark_to_string(feature_any->unique_id));
                        zMapWarnIfReached();
                      }
                  }

                if(full_data->end_callback && full_data->status == ZMAP_CONTEXT_EXEC_STATUS_OK)
                  {
                    if((full_data->status = (full_data->end_callback)(key, data,
                                                                      full_data->callback_data,
                                                                      &(full_data->error_string))) == ZMAP_CONTEXT_EXEC_STATUS_ERROR)
                      full_data->stopped_at = feature_type;
                    else if(full_data->status == ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND)
                      full_data->status = ZMAP_CONTEXT_EXEC_STATUS_OK;
                  }
              }
              break;
            case ZMAPFEATURE_STRUCT_FEATURE:
              /* No children here. can't possibly go further down, so no end-callback either. */
              break;
            case ZMAPFEATURE_STRUCT_INVALID:
            default:
              zMapWarnIfReached();
              break;
            }
        }
      else if(full_data->status == ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND)
        full_data->status = ZMAP_CONTEXT_EXEC_STATUS_OK;
      else
        {
          full_data->stopped_at = feature_type;
        }
    }

  if(full_data->status & ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE)
    {
      remove_from_hash   = TRUE;
      full_data->status ^= (full_data->status & ZMAP_CONTEXT_EXEC_NON_ERROR);/* clear only non-error bits */
    }

  return remove_from_hash;
}


static void postExecuteProcess(ContextExecute execute_data)
{

  if(execute_data->status >= ZMAP_CONTEXT_EXEC_STATUS_ERROR)
    {
      zMapLogCritical("ContextExecute: Error, function stopped @ level %d, message = %s",
                      execute_data->stopped_at,
                      execute_data->error_string);
    }

  return ;
}

gboolean zMapFeatureContextGetMasterAlignSpan(ZMapFeatureContext context,int *start,int *end)
{
  gboolean res = start && end;

  if(start)
    *start = context->master_align->sequence_span.x1;
  if(end)
    *end = context->master_align->sequence_span.x2;

  return(res);
}


static void feature_context_execute_full(ZMapFeatureAny feature_any,
                                         ZMapFeatureLevelType stop,
                                         ZMapGDataRecurseFunc start_callback,
                                         ZMapGDataRecurseFunc end_callback,
                                         gboolean use_remove,
                                         gboolean use_steal,
                                         gboolean catch_hash,
                                         gpointer data)
{
  ContextExecuteStruct full_data = {0};
  ZMapFeatureContext context = NULL;

  if(stop != ZMAPFEATURE_STRUCT_INVALID)
    {
      context = (ZMapFeatureContext)zMapFeatureGetParentGroup(feature_any, ZMAPFEATURE_STRUCT_CONTEXT);
      full_data.start_callback = start_callback;
      full_data.end_callback   = end_callback;
      full_data.callback_data  = data;
      full_data.stop           = stop;
      full_data.stopped_at     = ZMAPFEATURE_STRUCT_INVALID;
      full_data.status         = ZMAP_CONTEXT_EXEC_STATUS_OK;
      full_data.use_remove     = use_remove;
      full_data.use_steal      = use_steal;
      full_data.catch_hash     = catch_hash;

      executeDataForeachFunc(GINT_TO_POINTER(context->unique_id), context, &full_data);

      postExecuteProcess(&full_data);
    }

  return ;
}


/*!
 * \brief Callback called on every child in a FeatureAny.
 *
 * For each featureset, compares its id to the id in the user
 * data and if it matches it sets the result in the user data.
 */
static ZMapFeatureContextExecuteStatus getFeaturesetFromIdCB(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  GetFeaturesetCBData cb_data = (GetFeaturesetCBData) user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      if (feature_any->unique_id == cb_data->set_id)
        cb_data->featureset = (ZMapFeatureSet)feature_any;
      break;

    default:
      break;
    };

  return status;
}


/*!
 * \brief Callback called on every child in a FeatureAny.
 *
 * For each featureset, compares its id to the id in the user
 * data and if it matches it sets the result in the user data.
 */
static ZMapFeatureContextExecuteStatus getFeatureFromIdCB(GQuark key,
                                                          gpointer data,
                                                          gpointer user_data,
                                                          char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  GetFeaturesetCBData cb_data = (GetFeaturesetCBData) user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      if (feature_any->unique_id == cb_data->set_id)
        cb_data->featureset = (ZMapFeatureSet)feature_any;
      break;

    default:
      break;
    };

  return status;
}




/* This erase function actually moves features from the current context to a diff context
 * in preparation for removing the diff context. This is better because then we know
 * what we have removed for redrawing purposes. */
static ZMapFeatureContextExecuteStatus eraseContextCB(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData merge_data = (MergeContextData)user_data;
  ZMapFeatureAny  feature_any = (ZMapFeatureAny)data;
  ZMapFeatureAny erased_feature_any;
  ZMapFeature erased_feature;
  gboolean erase_feature_set = FALSE ;


  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
        break;
      }
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        merge_data->current_view_align =
          (ZMapFeatureAny)(zMapFeatureContextGetAlignmentByID(merge_data->view_context, key)) ;

        break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        merge_data->current_view_block = zMapFeatureParentGetFeatureByID(merge_data->current_view_align, key) ;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        merge_data->current_view_set = zMapFeatureParentGetFeatureByID(merge_data->current_view_block, key) ;

        /* Note that we fall through _IF_ caller specified just a feature set with no children because
         * then we delete all features in the set (but not the set itself as this is usually
         * needed for later features). */
        if (!zMapFeatureAnyHasChildren(feature_any))
          erase_feature_set = TRUE ;
        else
          break ;

      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeatureAny lookup_parent ;

        /* look up in the current */
        if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
          lookup_parent = merge_data->current_view_block ;
        else
          lookup_parent = merge_data->current_view_set ;

        if (!lookup_parent
            || !(erased_feature_any = zMapFeatureParentGetFeatureByID(lookup_parent, key)))
          {
            zMapLogWarning("Feature \"%s\" absent from current context, cannot delete.",
                           g_quark_to_string(key)) ;


            /* WHAT DOES THIS COMMENT MEAN !!! */

            /* no ...
             * leave in the erase context.
             */
          }
        else
          {
            if (merge_debug_G)
              zMapLogWarning("%s","\tFeature in erase and current contexts...");

            /* insert into the diff context.
             * BUT, need to check if the parents exist in the diff context first.
             */
            if (!merge_data->current_diff_align)
              {
                if (merge_debug_G)
                  zMapLogWarning("%s","\tno parent align... creating align in diff");

                merge_data->current_diff_align
                  = zmapFeatureAnyCreateFeature(merge_data->current_view_align->struct_type,
                                                NULL,
                                                merge_data->current_view_align->original_id,
                                                merge_data->current_view_align->unique_id,
                                                NULL) ;

                zMapFeatureContextAddAlignment(merge_data->diff_context,
                                               (ZMapFeatureAlignment)merge_data->current_diff_align,
                                               FALSE);
              }

            if (!merge_data->current_diff_block)
              {
                if (merge_debug_G)
                  zMapLogWarning("%s","\tno parent block... creating block in diff");

                merge_data->current_diff_block
                  = zmapFeatureAnyCreateFeature(merge_data->current_view_block->struct_type,
                                                NULL,
                                                merge_data->current_view_block->original_id,
                                                merge_data->current_view_block->unique_id,
                                                NULL) ;

                zMapFeatureAlignmentAddBlock((ZMapFeatureAlignment)merge_data->current_diff_align,
                                             (ZMapFeatureBlock)merge_data->current_diff_block);
              }

            if (!merge_data->current_diff_set)
              {
                ZMapFeatureSet feature_set ;

                if (merge_debug_G)
                  zMapLogWarning("%s","\tno parent set... creating set in diff");

                merge_data->current_diff_set
                  = zmapFeatureAnyCreateFeature(merge_data->current_view_set->struct_type,
                                                NULL,
                                                merge_data->current_view_set->original_id,
                                                merge_data->current_view_set->unique_id,
                                                NULL) ;

                feature_set = (ZMapFeatureSet)(merge_data->current_diff_set) ;

                feature_set->style = ((ZMapFeatureSet)(merge_data->current_view_set))->style ;

                zMapFeatureBlockAddFeatureSet((ZMapFeatureBlock)merge_data->current_diff_block,
                                              (ZMapFeatureSet)merge_data->current_diff_set);
              }

            if(merge_debug_G)
              zMapLogWarning("%s","\tmoving feature from current to diff context ... removing ... and inserting.");

            /* remove from the current context either a single feature or all features.*/
            if (!erase_feature_set)
              {
                erased_feature = (ZMapFeature)erased_feature_any ;

                zMapFeatureSetRemoveFeature((ZMapFeatureSet)merge_data->current_view_set,
                                            erased_feature);
                zMapFeatureSetAddFeature((ZMapFeatureSet)merge_data->current_diff_set,
                                         erased_feature) ;
              }
            else
              {
                ZMapFeatureSet feature_set ;
                GHashTableIter iter;
                gpointer key, value;

                feature_set = (ZMapFeatureSet)(merge_data->current_view_set) ;

                /* hash_table_iter is the safe way to iterate through a hash table while
                 * removing members. */
                g_hash_table_iter_init(&iter, feature_set->features) ;

                while (g_hash_table_iter_next(&iter, &key, &value))
                  {
                    ZMapFeature feature = (ZMapFeature)(value) ;

                    g_hash_table_iter_steal(&iter) ;

                    zMapFeatureSetAddFeature((ZMapFeatureSet)merge_data->current_diff_set,
                                             feature);
                  }
              }

            status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE ;
          }

        break;
      }

    default:
      {
        zMapWarnIfReached() ;

        break;
      }
    }

  return status ;
}





/* What is the purpose of this function...why is it not documented....???????? */
static ZMapFeatureContextExecuteStatus destroyIfEmptyContextCB(GQuark key,
                                                               gpointer data,
                                                               gpointer user_data,
                                                               char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  MergeContextData        merge_data = (MergeContextData)user_data;
  ZMapFeatureAny         feature_any = (ZMapFeatureAny)data;
  ZMapFeatureAlignment feature_align;
  ZMapFeatureBlock     feature_block;
  ZMapFeatureSet         feature_set;

  if(merge_debug_G)
    zMapLogWarning("checking %s", g_quark_to_string(feature_any->unique_id));

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      feature_align = (ZMapFeatureAlignment)feature_any;
      if (g_hash_table_size(feature_align->blocks) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty align ... destroying");
          zMapFeatureAlignmentDestroy(feature_align, TRUE);
#endif /* MESSES_UP_HASH */
          status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_align =
        merge_data->current_view_align = NULL;
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if (g_hash_table_size(feature_block->feature_sets) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty block ... destroying");
          zMapFeatureBlockDestroy(feature_block, TRUE);
#endif /* MESSES_UP_HASH */
          status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_block =
        merge_data->current_view_block = NULL;
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      feature_set = (ZMapFeatureSet)feature_any;
      if (g_hash_table_size(feature_set->features) == 0)
        {
#ifdef MESSES_UP_HASH
          if(merge_debug_G)
            zMapLogWarning("%s","\tempty set ... destroying");
          zMapFeatureSetDestroy(feature_set, TRUE);
#endif /* MESSES_UP_HASH */
          status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;
        }
      merge_data->current_diff_set =
        merge_data->current_view_set = NULL;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      break;
    default:
      break;
    }


  return status ;
}



static ZMapFeatureContextExecuteStatus addEmptySets(GQuark key,
                                                    gpointer data,
                                                    gpointer user_data,
                                                    char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  MergeContextData merge_data = (MergeContextData)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;


  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break ;

    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      break;

    case ZMAPFEATURE_STRUCT_BLOCK:
      zmapFeatureBlockAddEmptySets((ZMapFeatureBlock)(feature_any), (ZMapFeatureBlock)(feature_any), merge_data->req_featuresets);
      break;

    default:
      {
        zMapWarnIfReached() ;
        break;
      }
    }

  return status;
}



/* It's very important to note that the diff context hash tables _do_not_ have destroy functions,
 * this is what prevents them from freeing their children. */
static ZMapFeatureContextExecuteStatus mergePreCB(GQuark key,
                                                  gpointer data,
                                                  gpointer user_data,
                                                  char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  MergeContextData merge_data = (MergeContextData)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gboolean have_new = FALSE, children = FALSE ;
  ZMapFeatureAny *view_path_parent_ptr;
  ZMapFeatureAny *view_path_ptr = NULL;
  ZMapFeatureAny *diff_path_parent_ptr;
  ZMapFeatureAny *diff_path_ptr;

  /* THE CODE BELOW ALL NEEDS FACTORISING, START WITH THE SWITCH SETTING SOME
     COMMON POINTERS FOR THE FOLLOWING CODE..... */

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_FEATURE:
      view_path_parent_ptr = &(merge_data->current_view_set);
      view_path_ptr        = NULL;
      diff_path_parent_ptr = &(merge_data->current_diff_set);
      diff_path_ptr        = NULL;
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      view_path_parent_ptr = (ZMapFeatureAny *)(&(merge_data->view_context));
      view_path_ptr        = &(merge_data->current_view_align);
      diff_path_parent_ptr = (ZMapFeatureAny *)(&(merge_data->diff_context));
      diff_path_ptr        = &(merge_data->current_diff_align);
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      view_path_parent_ptr = &(merge_data->current_view_align);
      view_path_ptr        = &(merge_data->current_view_block);
      diff_path_parent_ptr = &(merge_data->current_diff_align);
      diff_path_ptr        = &(merge_data->current_diff_block);
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      view_path_parent_ptr = &(merge_data->current_view_block);
      view_path_ptr        = &(merge_data->current_view_set);
      diff_path_parent_ptr = &(merge_data->current_diff_block);
      diff_path_ptr        = &(merge_data->current_diff_set);
      break;
    default:
      zMapWarnIfReached();
    }


  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break ;

    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
#ifdef NO_IDEA_WHAT_SHOULD_HAPPEN_HERE
        gboolean is_master_align = FALSE;
#endif

        /* Annoyingly we have a issue with alignments */
        if (feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN)
          {
            ZMapFeatureContext context = (ZMapFeatureContext)(feature_any->parent);
            if (context->master_align == (ZMapFeatureAlignment)feature_any)
              {
#ifdef NO_IDEA_WHAT_SHOULD_HAPPEN_HERE
                is_master_align = TRUE;
#endif
                context->master_align = NULL;
              }
          }

        /* Only need to do anything if this level has children. */
        if (feature_any->children)
          {
            ZMapFeatureAny diff_feature_any;

            children = TRUE;/* We have children. */

            /* This removes the link in the "from" feature to it's parent
             * and the status makes sure it gets removed from the parent's
             * hash on return. */
            feature_any->parent = NULL;
            status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;

            if (!(*view_path_ptr = zMapFeatureParentGetFeatureByID(*view_path_parent_ptr,
                                                                   feature_any->unique_id)))
              {
                /* If its new we can simply copy a pointer over to the diff context
                 * and stop recursing.... */


                merge_data->new_features = have_new = TRUE;/* This is a NEW feature. */

                /* We would use featureAnyAddFeature, but it does another
                 * g_hash_table_lookup... */
                g_hash_table_insert((*view_path_parent_ptr)->children,
                                    zmapFeature2HashKey(feature_any),
                                    feature_any);
                feature_any->parent = *view_path_parent_ptr;
                /* end of featureAnyAddFeature bit... */

                /* update the path */
                *view_path_ptr      = feature_any;

                diff_feature_any    = feature_any;

                diff_feature_any->parent = *view_path_parent_ptr;

                status |= ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;

                /* Not descending to feature level so need to record number of features. */
                if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
                  merge_data->feature_count += g_hash_table_size(feature_any->children) ;

              }
            else
              {
                have_new = FALSE;/* Nothing new here */
                /* If the feature is there we need to copy it and then recurse down until
                 * we get to the individual feature level. */
                diff_feature_any = zmapFeatureAnyCopy(feature_any, NULL);

                zmapFeatureAnyAddToDestroyList(merge_data->diff_context, diff_feature_any);
              }


            // mh17:
            // 1) featureAnyAddFeature checks to see if it's there first, which we just did :-(
            // 2) look at the comment 25 lines above about not using featureAnyAddFeature
            zmapFeatureAnyAddFeature(*diff_path_parent_ptr, diff_feature_any);

            /* update the path */
            *diff_path_ptr = diff_feature_any;

            /* keep diff feature -> parent up to date with the view parent */
            if(have_new)
              (*diff_path_ptr)->parent = *view_path_parent_ptr;

#if MH17_OLD_CODE
            if (feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK &&
                (*view_path_ptr)->unique_id == feature_any->unique_id)
              {
                ((ZMapFeatureBlock)(*view_path_ptr))->features_start =
                  ((ZMapFeatureBlock)(feature_any))->features_start;
                ((ZMapFeatureBlock)(*view_path_ptr))->features_end   =
                  ((ZMapFeatureBlock)(feature_any))->features_end;
              }
#else
            // debugging reveals that features start and end are 1,0 in the new data and sensible in the old
            // maybe 1,0 gets reprocessed into start,end?? so let's leave it as it used to run
            // BUT: this code should only run if not a new block 'cos if it's new then we've just
            // assigned the pointer
            // this is a block which may hold a sequence, the dna featureset is another struct
            if (!have_new && feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK &&
                (*view_path_ptr)->unique_id == feature_any->unique_id)
              {
                ZMapFeatureBlock vptr = (ZMapFeatureBlock)(*view_path_ptr);
                ZMapFeatureBlock feat = (ZMapFeatureBlock) feature_any;

#if MH17_THIS_IS_WRONG_FOR_REQ_FROM_MARK
                /* sub block gets merging into a bigger one which still has the original range
                 * featuresets have a span which gets added to the list in the original context featureset
                 */
                vptr->block_to_sequence.block.x1 = feat->block_to_sequence.block.x1;
                vptr->block_to_sequence.block.x2 = feat->block_to_sequence.block.x2;
#endif
                // mh17: DNA did not get merged from a non first source
                // by analogy with the above we need to copy the sequence too, in fact any Block only data
                // sequence contains a pointer to a (long) string - can we just copy it?
                // seems to work without double frees...
                if(!vptr->sequence.sequence)      // let's keep the first one, should be the same
                  {
                    //                memcpy(&vptr->block_to_sequence,&feat->block_to_sequence,sizeof(ZMapMapBlockStruct));
                    memcpy(&vptr->sequence,&feat->sequence,sizeof(ZMapSequenceStruct));
                  }
              }
#endif
          }

        /* general code stop */

        if (feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK)
          {
            /* we get only featuresest with data from the server */
            /* and record empty ones with a seq region list of requested ranges in the features context */
            zmapFeatureBlockAddEmptySets((ZMapFeatureBlock) feature_any, (ZMapFeatureBlock)(*diff_path_ptr), merge_data->req_featuresets);
            zmapFeatureBlockAddEmptySets((ZMapFeatureBlock) feature_any, (ZMapFeatureBlock)(*view_path_ptr), merge_data->req_featuresets);
          }

        if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
          {
            /* if we have a featureset there will be features (see zmapGFF2parser.c/makeNewfeature())
             * so view_path_ptr is valid here
             */

            ZMapFeatureContext context = merge_data->diff_context;
            //printf("adding %s (%p) with %d features to context %p\n", g_quark_to_string(feature_any->unique_id),*diff_path_ptr,g_hash_table_size((*diff_path_ptr)->children),context);

            context->src_feature_set_names = g_list_prepend(context->src_feature_set_names, GUINT_TO_POINTER(feature_any->unique_id));

            if(!have_new)
              {
                mergeFeatureSetLoaded((ZMapFeatureSet)*view_path_ptr, (ZMapFeatureSet) feature_any);
              }
          }

#ifdef NO_IDEA_WHAT_SHOULD_HAPPEN_HERE
        /* possibly nothing... unsure where the master alignment status [will] comes from. */
        /* and finish the alignment */
        if(is_master_align && feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN)
          {
            merge_data->diff_context->master_align = (ZMapFeatureAlignment)diff_feature_any;
          }
#endif /* NO_IDEA_WHAT_SHOULD_HAPPEN_HERE */

        break;
      }


    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        feature_any->parent = NULL;
        status = ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE;

        if (!(zMapFeatureParentGetFeatureByID(*view_path_parent_ptr, feature_any->unique_id)))
          {
            merge_data->new_features = have_new = TRUE ;
            merge_data->feature_count++ ;

            zmapFeatureAnyAddFeature(*diff_path_parent_ptr, feature_any) ;

            zmapFeatureAnyAddFeature(*view_path_parent_ptr, feature_any) ;


            if (merge_debug_G)
              zMapLogWarning("feature(%p)->parent = %p. current_view_set = %p",
                             feature_any, feature_any->parent, *view_path_parent_ptr) ;
          }
        else
          {
            have_new = FALSE ;
          }

        break;
      }

    default:
      {
        zMapWarnIfReached() ;

        break;
      }
    }

  if (merge_debug_G)
    zMapLogWarning("%s (%p) '%s' is %s and has %s",
                   zMapFeatureLevelType2Str(feature_any->struct_type),
                   feature_any,
                   g_quark_to_string(feature_any->unique_id),
                   (have_new == TRUE ? "new" : "old"),
                   (children ? "children and was added" : "no children and was not added"));
#if 0
  printf("%s (%p) '%s' is %s and has %s\n",
         zMapFeatureLevelType2Str(feature_any->struct_type),
         feature_any,
         g_quark_to_string(feature_any->unique_id),
         (have_new == TRUE ? "new" : "old"),
         (children ? "children and was added" : "no children and was not added"));
#endif


  return status;
}




static void mergeFeatureSetLoaded(ZMapFeatureSet view_set, ZMapFeatureSet new_set)
{
  GList *view_list,*new_list;
  ZMapSpan view_span,new_span,got_span;
  gboolean got = FALSE;

  if(view_set == new_set) /* loaded list transfered to view context, will not be freed */
    return;

  if(!view_set || !new_set)
    {
      return;
    }


  /* we expect to just add our seq region to the existing
   * may have to combine adjacent
   * and logically deal with overlaps, which should not happen
   *
   * new list can only have one region due to GFF constraints
   */

  view_list = view_set->loaded;
  new_list = new_set->loaded;


  if(!view_list)
    {
      view_set->loaded = new_set->loaded;
      zMapLogWarning("merge loaded %s had no loaded list", g_quark_to_string(view_set->unique_id));
    }
  if(!new_list)
    {
      zMapLogWarning("merge loaded %s had no loaded list", g_quark_to_string(new_set->unique_id));
      return;
    }


  new_span = (ZMapSpan) new_list->data;
  if(new_list->next)
    {
      /* due to GFF format we can only have one region */
      zMapLogWarning("unexpected multiple region in new context ignored in featureset %s", g_quark_to_string(new_set->unique_id));
    }

  /*
    this is disapointingly complex:

    how many cases? (13)

    | characters have no width
    x1 and x2 are inclusive

    |x1  x2|
    |vvvvvv|
    nnn  |      |
    nnn|      |
    n|nn    |
    |nnn   |
    |  nnn |
    |   nnn|
    |    nn|n
    |      |nnn
    |      |   nnn
    |nnnnnn|
    nnn|nnnnnn|
    |nnnnnn|nnn
    nnnn|nnnnnn|nnnn


    How many actions? (4)

    new    view  action

    end == x1-1  join
    end  < x1    insert before

    start <= x2+1 join

    try next, or insert after

    but we should also join up two view segemnts which implies processing twice:

    vvvv|     |vvvv
    |nnnnn|
    or:

    vvvvv|     |vvvvv|      |vvvvv
    nn|nnnnn|nnnnn|nnnnnn|nn

    so: insert or join once only, continue till end <= x2
  */

  while(view_list)
    {
      view_span = (ZMapSpan) view_list->data;

      if(got)     /* we are joining an already combined region */
        {
          if(got_span->x2 >= view_span->x1 - 1) /* join */
            {
              if(got_span->x2 < view_span->x2)
                got_span->x2 = view_span->x2;
            }

          if(got_span->x2 >= view_span->x2)   /* remove then continue */
            {
              GList *del = view_list;

              view_list = view_list->next;
              g_free(view_span);
              view_set->loaded = g_list_delete_link(view_set->loaded,del);
            }
          else /* there is a gap */
            {
              break;
            }
        }
      else
        {
          if(new_span->x2 + 1 == view_span->x1)     /* join then we are finished */
            {
              view_span->x1 = new_span->x1;
              got = TRUE;
              break;
            }
          if(new_span->x2 < view_span->x1)          /* insert before then we are finished */
            {
              view_set->loaded = g_list_insert_before(view_set->loaded,view_list,
                                                      g_memdup(new_span,sizeof(ZMapSpanStruct)));
              got = TRUE;
              break;
            }
          if(new_span->x1 <= view_span->x2 + 1)   /* join then continue */
            {
              if(view_span->x1 > new_span->x1)
                view_span->x1 = new_span->x1;
              if(view_span->x2 < new_span->x2)
                view_span->x2 = new_span->x2;
              got = TRUE;
            }
          if(got)
            got_span = view_span;

          view_list = view_list->next;
        }
    }

  if(!got)
    {
      view_set->loaded = g_list_append(view_set->loaded, g_memdup(new_span,sizeof(ZMapSpanStruct)));
    }


  return ;
}

