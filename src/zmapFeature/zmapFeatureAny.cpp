/*  File: zmapFeatureAny.cpp
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
 * Description: There are a number of different types of feature, 
 *              e.g. context, align, block etc. and the functions
 *              in this file provide a general interface to all
 *              of these types to create, modify and destroy them.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib.h>

#include <zmapFeature_P.hpp>


/* Currently if we use this we get seg faults so we must not be cleaning up properly somewhere... */
static gboolean USE_SLICE_ALLOC = TRUE ;
static gboolean LOG_MEM_CALLS = FALSE ;



/* Utility struct to store a feature id to search for and the resultant feature that is found */
typedef struct _FeatureSearchStruct
{
  GQuark search_id ;             /* feature id to search for */
  ZMapFeatureAny found_feature ; /* the feature we found */
} FeatureSearchStruct, *FeatureSearch ;



static void destroyContextSubparts(ZMapFeatureContext context) ;
static void destroyFeature(ZMapFeature feature) ;
static void destroyFeatureAnyShallow(gpointer data) ;
static gboolean withdrawFeatureAny(gpointer key, gpointer value, gpointer user_data) ;

static void logMemCalls(gboolean alloc, ZMapFeatureAny feature_any) ;
static void printDestroyDebugInfo(ZMapFeatureAny any, const char *who) ;

static ZMapFeatureContextExecuteStatus find_feature_by_id(GQuark key,
                                                          gpointer data,
                                                          gpointer user_data,
                                                          char **error_out) ;


// 
//             Globals
//    

static gboolean destroy_debug_G = FALSE ;

#ifdef FEATURES_NEED_MAGIC
ZMAP_MAGIC_NEW(feature_magic_G, ZMapFeatureAnyStruct) ;
#endif /* FEATURES_NEED_MAGIC */



//
//                               External interface routines.
//


gboolean zMapFeatureAnyHasMagic(ZMapFeatureAny feature_any)
{
  gboolean has_magic = TRUE;

#ifdef FEATURES_NEED_MAGIC
  has_magic = ZMAP_MAGIC_IS_VALID(feature_magic_G, feature_any->magic);
#endif

  return has_magic;
}



/* Make a copy of any feature, the feature is "stand alone", i.e. it has no parent
 * and no children, these are simply not copied, or any other dynamically allocated stuff ?? */
ZMapFeatureAny zMapFeatureAnyCopy(ZMapFeatureAny orig_feature_any)
{
  ZMapFeatureAny new_feature_any  = NULL ;

  if (orig_feature_any && zMapFeatureIsValid(orig_feature_any))
    new_feature_any = zmapFeatureAnyCopy(orig_feature_any, zmapDestroyFeatureAny) ;

  return new_feature_any ;
}


/* Returns TRUE if the feature could be found in the feature_set, FALSE otherwise. */
gboolean zMapFeatureAnyFindFeature(ZMapFeatureAny feature_set, ZMapFeatureAny feature)
{
  gboolean result = FALSE ;
  ZMapFeature hash_feature ;

  if (!feature_set || !feature )
    return result ;

  if ((hash_feature = (ZMapFeature)g_hash_table_lookup(feature_set->children, zmapFeature2HashKey(feature))))
    result = TRUE ;

  return result ;
}


/* Looks up a feature_id in the given parent. Returns the feature if found, NULL otherwise. */
ZMapFeatureAny zMapFeatureParentGetFeatureByID(ZMapFeatureAny feature_parent, GQuark feature_id)
{
  ZMapFeatureAny feature = NULL ;
  zMapReturnValIfFail(feature_parent, feature) ;

  feature = (ZMapFeatureAny)g_hash_table_lookup(feature_parent->children, GINT_TO_POINTER(feature_id)) ;

  return feature ;
}


/* Looks up a feature_id in ANY level of the child features of feature_any. Returns the feature if
 * found, NULL otherwise. If you know that feature_any is a direct parent of the feature you're
 * looking for use zMapFeatureParentGetFeatureByID instead as that is more effcient. */
ZMapFeatureAny zMapFeatureAnyGetFeatureByID(ZMapFeatureAny feature_any,
                                            GQuark feature_id,
                                            ZMapFeatureLevelType struct_type)
{
  ZMapFeatureAny feature = NULL ;
  zMapReturnValIfFail(feature_any && feature_id, feature) ;

  FeatureSearchStruct search_data = {feature_id, NULL} ;

  zMapFeatureContextExecute(feature_any,
                            struct_type,
                            find_feature_by_id,
                            &search_data);

  feature = search_data.found_feature ;

  return feature ;
}



gboolean zMapFeatureAnyRemoveFeature(ZMapFeatureAny feature_parent, ZMapFeatureAny feature)
{
  gboolean result = FALSE;

  if (zMapFeatureAnyFindFeature(feature_parent, feature))
    {
      result = g_hash_table_steal(feature_parent->children, zmapFeature2HashKey(feature)) ;
      feature->parent = NULL;

      switch(feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          break ;
        case ZMAPFEATURE_STRUCT_ALIGN:
          {
            ZMapFeatureContext context = (ZMapFeatureContext)feature_parent ;

            if (context->master_align == (ZMapFeatureAlignment)feature)
              context->master_align = NULL ;

            break ;
          }
        case ZMAPFEATURE_STRUCT_BLOCK:
          break ;
        case ZMAPFEATURE_STRUCT_FEATURESET:
          break ;
        case ZMAPFEATURE_STRUCT_FEATURE:
          break ;
        default:
                  zMapWarnIfReached() ;
          break ;
        }

      result = TRUE ;
    }

  return result ;
}


gboolean zMapFeatureAnyHasChildren(ZMapFeatureAny feature_any)
{
  gboolean has_children = FALSE ;

  if ((g_hash_table_size(feature_any->children)))
    has_children = TRUE ;

  return has_children ;
}



void zMapFeatureAnyDestroy(ZMapFeatureAny feature_any)
{

  if (!zMapFeatureIsValid(feature_any))
    return ;

  zmapDestroyFeatureAnyWithChildren(feature_any, TRUE) ;

  return ;
}


void zMapFeatureAnyDestroyShallow(ZMapFeatureAny feature_any)
{

  if (!zMapFeatureIsValid(feature_any))
    return ;

  destroyFeatureAnyShallow(feature_any) ;

  return ;
}






// 
//                     Package routines
//    

/* Hooks up the feature into the feature_sets children and makes feature_set the parent
 * of feature _if_ the feature is not already in feature_set. */
gboolean zmapFeatureAnyAddFeature(ZMapFeatureAny feature_any, ZMapFeatureAny feature)
{
  gboolean result = FALSE ;

  if (!zMapFeatureAnyFindFeature(feature_any, feature))
    {
      g_hash_table_insert(feature_any->children, zmapFeature2HashKey(feature), feature) ;

      feature->parent = feature_any ;

        if(feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
        {
        /* as features have styles stored indrectly in their containing featureset
         * we must assign this here as this is called from merge
         * the actual style struct will be the same one but we access it via an indirect link
         * and after the merge that pointer woudl be invalid
         */
        /* NOTE also called from locus code in viewremote receive, but is benign */
        ZMapFeature feat = (ZMapFeature) feature;
        ZMapFeatureSet feature_set = (ZMapFeatureSet) feature_any;
        feat->style = & feature_set->style;
        }

      result = TRUE ;
    }

  return result ;
}

void zmapFeatureAnyAddToDestroyList(ZMapFeatureContext context, ZMapFeatureAny feature_any)
{
  if (!context || !feature_any)
    return ;

  g_hash_table_insert(context->elements_to_destroy, zmapFeature2HashKey(feature_any), feature_any) ;

  return ;
}


/* Allocate a feature structure of the requested type, filling in the feature any fields. */
ZMapFeatureAny zmapFeatureAnyCreateFeature(ZMapFeatureLevelType struct_type,
                                           ZMapFeatureAny parent,
                                           GQuark original_id, GQuark unique_id,
                                           GHashTable *children)
{
  ZMapFeatureAny feature_any = NULL ;
  gulong nbytes = 0 ;

  switch(struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      nbytes = sizeof(ZMapFeatureContextStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      nbytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      nbytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      nbytes = sizeof(ZMapFeatureSetStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:
      nbytes = sizeof(ZMapFeatureStruct) ;
      break ;
    default:
      zMapWarnIfReached() ;
      break ;
    }

  if (nbytes > 0)
    {
      if (USE_SLICE_ALLOC)
        feature_any = (ZMapFeatureAny)g_slice_alloc0(nbytes) ;
      else
        feature_any = (ZMapFeatureAny)g_malloc0(nbytes) ;

#ifdef FEATURES_NEED_MAGIC
      feature_any->magic = feature_magic_G;
#endif

      feature_any->struct_type = struct_type ;

      logMemCalls(TRUE, feature_any) ;

      if (struct_type != ZMAPFEATURE_STRUCT_CONTEXT)
        feature_any->parent = parent ;

      feature_any->original_id = original_id ;
      feature_any->unique_id = unique_id ;

      if (struct_type != ZMAPFEATURE_STRUCT_FEATURE)
        {
          if (children)
            feature_any->children = children ;
          else
            feature_any->children = g_hash_table_new_full(NULL, NULL, NULL, zmapDestroyFeatureAny) ;
        }
    }

  return feature_any ;
}



/* Copy any type of feature.....a copy contructor only not as good.... */
ZMapFeatureAny zmapFeatureAnyCopy(ZMapFeatureAny orig_feature_any, GDestroyNotify destroy_cb)
{
  ZMapFeatureAny new_feature_any  = NULL ;
  zMapReturnValIfFail(orig_feature_any, new_feature_any) ;
  zMapReturnValIfFail(orig_feature_any->struct_type == ZMAPFEATURE_STRUCT_CONTEXT ||
                      orig_feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN ||
                      orig_feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK ||
                      orig_feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET ||
                      orig_feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE,
                      new_feature_any) ;

  guint bytes = 0 ;

  /* Copy the original struct and set common fields. */
  switch(orig_feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      bytes = sizeof(ZMapFeatureContextStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      bytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      bytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      bytes = sizeof(ZMapFeatureSetStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:
      bytes = sizeof(ZMapFeatureStruct) ;
      break ;
    default:
      zMapWarnIfReached();
      break;
    }


  if (USE_SLICE_ALLOC)
    {
      new_feature_any = (ZMapFeatureAny)g_slice_alloc0(bytes) ;
      g_memmove(new_feature_any, orig_feature_any, bytes) ;
    }
  else
    {
      new_feature_any = (ZMapFeatureAny)g_memdup(orig_feature_any, bytes) ;
    }
  logMemCalls(TRUE, new_feature_any) ;



  /* We DO NOT copy children or parents... */
  new_feature_any->parent = NULL ;
  if (new_feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    new_feature_any->children = g_hash_table_new_full(NULL, NULL, NULL, destroy_cb) ;


  /* Fill in the fields unique to each struct type. */
  switch(new_feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
        ZMapFeatureContext new_context = (ZMapFeatureContext)new_feature_any ;

        new_context->elements_to_destroy = NULL ;

        new_context->req_feature_set_names = NULL ;

        new_context->src_feature_set_names = NULL ;

        new_context->master_align = NULL ;

        break ;
      }
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        break;
      }
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock new_block = (ZMapFeatureBlock)new_feature_any ;

        new_block->sequence.type = ZMAPSEQUENCE_NONE ;
        new_block->sequence.length = 0 ;
        new_block->sequence.sequence = NULL ;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet new_set = (ZMapFeatureSet) new_feature_any;

        /* MH17: this is for a copy of the featureset to be added to the diff context on merge
         * we could set this to NULL to avoid a double free
         * but there are a few calls here from WindowRemoteReceive, viewRemoteReceive, WindowDNA, WindowDraw
         * and while it's tedious to copy the list it save a lot of staring at code
         */

        GList *l,*copy_list = NULL;
        ZMapSpan span;

        for(l = new_set->loaded;l;l = l->next)
          {
            span = (ZMapSpan)g_memdup(l->data,sizeof(ZMapSpanStruct));
            copy_list = g_list_append(copy_list,span);    /* must preserve the order */
          }

        new_set->loaded = copy_list;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature new_feature = (ZMapFeature)new_feature_any,
          orig_feature = (ZMapFeature)orig_feature_any ;

        zmapFeatureBasicCopyFeature(orig_feature, new_feature) ;

        if (new_feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
          {
            zmapFeatureAlignmentCopyFeature(orig_feature, new_feature) ;
          }
        else if (new_feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
          {
            zmapFeatureTranscriptCopyFeature(orig_feature, new_feature) ;
          }

        break ;
      }

    default:
      {
        zMapWarnIfReached() ;

        break;
      }
    }

  return new_feature_any ;
}



/* A GDestroyNotify() function called from g_hash_table_destroy() to get rid of all children
 * in the hash. */
void zmapDestroyFeatureAny(gpointer data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gulong nbytes = 0 ;

  zMapReturnIfFail(feature_any);
  zMapReturnIfFail(zMapFeatureAnyHasMagic(feature_any));

  if (destroy_debug_G && feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    printDestroyDebugInfo(feature_any, "destroyFeatureAny") ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      nbytes = sizeof(ZMapFeatureContextStruct) ;
      destroyContextSubparts((ZMapFeatureContext)feature_any) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      nbytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      nbytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = (ZMapFeatureSet) feature_any;
        GList *l;

        if(feature_set->masker_sorted_features)
          g_list_free(feature_set->masker_sorted_features);
        feature_set->masker_sorted_features = NULL;

        if(feature_set->loaded)
          {
            for(l = feature_set->loaded;l;l = l->next)
              {
                g_free(l->data);
              }
            g_list_free(feature_set->loaded);
          }
        feature_set->loaded = NULL;

        nbytes = sizeof(ZMapFeatureSetStruct) ;

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      nbytes = sizeof(ZMapFeatureStruct) ;

      destroyFeature((ZMapFeature)feature_any) ;
      break;
    default:
      zMapWarnIfReached();
      break;
    }

  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    {
      g_hash_table_destroy(feature_any->children) ;
    }


  logMemCalls(FALSE, feature_any) ;

  /* nbytes is zero if we were given an invalid struct type - shouldn't
   * happen but check anyway */
  if (nbytes)
    {
      memset(feature_any, 0, nbytes) ;    /* Make sure mem for struct is useless. */
      if (USE_SLICE_ALLOC)
        g_slice_free1(nbytes, feature_any) ;
    }

  if (!USE_SLICE_ALLOC)
    {
      g_free(feature_any) ;
    }

  return ;
}


gboolean zmapDestroyFeatureAnyWithChildren(ZMapFeatureAny feature_any, gboolean free_children)
{
  gboolean result = FALSE ;

  if (!feature_any)
    return result ;
  result = TRUE ;

  /* I think this is equivalent to the below code.... */

  /* If we have a parent then remove ourselves from the parent. */
  if (feature_any->parent)
    {
      /* splice out the feature_any from parent */
      result = g_hash_table_steal(feature_any->parent->children, zmapFeature2HashKey(feature_any)) ;
    }

  /* If we have children but they should not be freed, then remove them before destroying the
   * feature, otherwise the children will be destroyed. */
  if (result && (feature_any->children && !free_children))
    {
      if ((g_hash_table_foreach_steal(feature_any->children,
      withdrawFeatureAny, (gpointer)feature_any)))
        {
          result = TRUE ;
        }
    }

  /* Now destroy the feature. */
  if (result)
    {
      zmapDestroyFeatureAny((gpointer)feature_any) ;

      result = TRUE ;
    }

  return result ;
}




/*
 *            Internal routines.
 */


/* Frees resources that are unique to a context, resources common to all features
 * are freed by a common routine. */
static void destroyContextSubparts(ZMapFeatureContext context)
{

  /* diff contexts are different because they are a mixture of pointers to subtrees in another
   * context, individual features in another context and copied elements in subtrees. */
  if (context->diff_context)
    {
      /* Remove the copied elements. */
      g_hash_table_destroy(context->elements_to_destroy) ;
    }


  if (context->req_feature_set_names)
    {
      g_list_free(context->req_feature_set_names) ;
      context->req_feature_set_names = NULL ;
    }
  if (context->src_feature_set_names)
    {
      g_list_free(context->src_feature_set_names) ;
      context->src_feature_set_names = NULL ;
    }
  return ;
}



static void destroyFeature(ZMapFeature feature)
{
  zmapFeatureBasicDestroyFeature(feature) ;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    zmapFeatureTranscriptDestroyFeature(feature) ;
  else if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    zmapFeatureAlignmentDestroyFeature(feature) ;

  return ;
}


static void destroyFeatureAnyShallow(gpointer data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gulong nbytes = 0 ;

  zMapReturnIfFail(feature_any);
  zMapReturnIfFail(zMapFeatureAnyHasMagic(feature_any));

  if (destroy_debug_G && feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    printDestroyDebugInfo(feature_any, "destroyFeatureAnyShallow") ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      nbytes = sizeof(ZMapFeatureContextStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_ALIGN:
      nbytes = sizeof(ZMapFeatureAlignmentStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      nbytes = sizeof(ZMapFeatureBlockStruct) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      nbytes = sizeof(ZMapFeatureSetStruct) ;
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      nbytes = sizeof(ZMapFeatureStruct) ;
      break;
    default:
      zMapWarnIfReached();
      break;
    }

  logMemCalls(FALSE, feature_any) ;

  /* nbytes is zero if we were given an invalid struct type - shouldn't
   * happen but check anyway */
  if (nbytes)
    {
      memset(feature_any, 0, nbytes) ;    /* Make sure mem for struct is useless. */
      if (USE_SLICE_ALLOC)
        g_slice_free1(nbytes, feature_any) ;
    }

  if (!USE_SLICE_ALLOC)
    {
      g_free(feature_any) ;
    }

  return ;
}


/* A GHRFunc() called by hash steal to remove items from hash _without_ calling the item destroy routine. */
static gboolean withdrawFeatureAny(gpointer key, gpointer value, gpointer user_data)
{
  gboolean remove = TRUE ;


  /* We want to remove all items so always return TRUE. */

  return remove ;
}


static void logMemCalls(gboolean alloc, ZMapFeatureAny feature_any)
{
  const char *func ;

  if (LOG_MEM_CALLS)
    {
      if (USE_SLICE_ALLOC)
        {
          if (alloc)
            func = "g_slice_alloc0" ;
          else
            func = "g_slice_free1" ;
        }
      else
        {
          if (alloc)
            func = "g_malloc" ;
          else
            func = "g_free" ;
        }

      zMapLogWarning("%s: %s at %p", func, zMapFeatureLevelType2Str(feature_any->struct_type), feature_any) ;
    }

  return ;
}

static void printDestroyDebugInfo(ZMapFeatureAny feature_any, const char *who)
{
  int length = 0 ;

  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    length = g_hash_table_size(feature_any->children) ;

  if(destroy_debug_G)
    zMapLogWarning("%s: (%p) '%s' datalist size %d", who, feature_any, g_quark_to_string(feature_any->unique_id), length) ;

  return ;
}



/* Callback used to determine whether the current feature has the id in the user data and
 * if so to set the result in the user data.
 */
static ZMapFeatureContextExecuteStatus find_feature_by_id(GQuark key,
                                                          gpointer data,
                                                          gpointer user_data,
                                                          char **error_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;

  ZMapFeatureAny any = (ZMapFeatureAny)data ;
  FeatureSearch search_data = (FeatureSearch)user_data ;

  if (any->unique_id == search_data->search_id)
    {
      search_data->found_feature = any ;
      status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND ; /* stop searching */
    }

  return status ;
}

