/*  File: zmapFeatureContext.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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

#include <ZMap/zmap.h>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapDNA.h>
#include <zmapFeature_P.h>




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
  GHashTable *styles ;
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




/* 
 *                    External routines
 */


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
void zMapFeatureContextReverseComplement(ZMapFeatureContext context, GHashTable *styles)
{
  RevCompDataStruct cb_data ;

  cb_data.styles = styles ;

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
      else
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
    query_context = NULL,
    query_align   = NULL,
    query_block   = NULL,
    query_set     = NULL;

  /* Go up in the query feature's tree to the top, recording on the way */
  feature_ptr = query_feature;
  while(feature_ptr && feature_ptr->struct_type > ZMAPFEATURE_STRUCT_CONTEXT)
    {
      feature_ptr = zMapFeatureGetParentGroup(feature_ptr, ZMapFeatureLevelType(feature_ptr->struct_type - 1));
      switch(feature_ptr->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          query_context = feature_ptr;
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


/* provide access to private macro, NB start is not used but defined by the macro */
int zmapFeatureRevCompCoord(int *coord, const int start, const int end)
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

  for (i = 0; i < spans->len; i++)
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

      for (i = 0; i < feature->feature.homol.align->len; i++)
        {
          ZMapAlignBlock align ;

          align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;

          zmapFeatureRevComp(start_coord, end_coord, &align->t1, &align->t2) ;
          /*! \todo #warning why does this not change t_strand? */
        }

      zMapFeatureSortGaps(feature->feature.homol.align) ;

        if(feature->feature.homol.sequence && *feature->feature.homol.sequence)/* eg if provided in GFF (BAM) */
          {
            if (feature->feature.homol.length != strlen(feature->feature.homol.sequence))
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
