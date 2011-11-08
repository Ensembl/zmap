/*  File: zmapFeatureContext.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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



/* This isn't ideal, but there is code calling the getDNA functions
 * below, not checking return value... */
#define NO_DNA_STRING "<No DNA>"

#define ZMAP_CONTEXT_EXEC_NON_ERROR (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE | ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND)
#define ZMAP_CONTEXT_EXEC_RECURSE_OK (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE)
#define ZMAP_CONTEXT_EXEC_RECURSE_FAIL (~ ZMAP_CONTEXT_EXEC_RECURSE_OK)

typedef struct
{
  /* Input dna string, note that start != 1 where we are looking at a sub-part of an assembly
   * but start/end must be inside the assembly start/end. */
  char *dna_in;
  int dna_start ;
  int start, end ;

  /* Output dna string. */
  GString *dna_out ;
}FeatureSeqFetcherStruct, *FeatureSeqFetcher;


typedef struct
{
  ZMapGDataRecurseFunc  start_callback;
  ZMapGDataRecurseFunc  end_callback;
  gpointer              callback_data;
  char                 *error_string;
  ZMapFeatureStructType stop;
  ZMapFeatureStructType stopped_at;
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
} RevCompDataStruct, *RevCompData ;



static void revCompFeature(ZMapFeature feature, int start_coord, int end_coord);
static ZMapFeatureContextExecuteStatus revCompFeaturesCB(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out);
static void revcompSpan(GArray *spans, int start_coord, int seq_end) ;

static char *getFeatureBlockDNA(ZMapFeatureAny feature_any, int start_in, int end_in, gboolean revcomp) ;
static char *fetchBlockDNAPtr(ZMapFeatureAny feature, ZMapFeatureBlock *block_out) ;
static char *getDNA(char *dna, int start, int end, gboolean revcomp) ;
static gboolean coordsInBlock(ZMapFeatureBlock block, int *start_out, int *end_out) ;

static gboolean executeDataForeachFunc(gpointer key, gpointer data, gpointer user_data);
static void fetch_exon_sequence(gpointer exon_data, gpointer user_data);
static void postExecuteProcess(ContextExecute execute_data);
static void copyQuarkCB(gpointer data, gpointer user_data) ;

static void feature_context_execute_full(ZMapFeatureAny feature_any,
					 ZMapFeatureStructType stop,
					 ZMapGDataRecurseFunc start_callback,
					 ZMapGDataRecurseFunc end_callback,
					 gboolean use_remove,
					 gboolean use_steal,
					 gboolean catch_hash,
					 gpointer data) ;





static gboolean catch_hash_abuse_G = TRUE;



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

  //zMapLogWarning("rev comp, parent span = %d -> %d",context->parent_span.x1,context->parent_span.x2);

  /* Because this doesn't allow for execution at context level ;( */
  zMapFeatureContextExecute((ZMapFeatureAny)context,
                            ZMAPFEATURE_STRUCT_FEATURE,
                            revCompFeaturesCB,
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

void zMapFeatureReverseComplementCoords(ZMapFeatureBlock block, int *start_inout, int *end_inout)
{
  int start, end, my_start, my_end ;
  ZMapFeatureContext context = (ZMapFeatureContext) block->parent->parent;

  start = context->parent_span.x1 ;
  end   = context->parent_span.x2 ;

  my_start = *start_inout ;
  my_end = *end_inout ;

  zmapFeatureRevComp(Coord, start, end, my_start, my_end) ;

  *start_inout = my_start ;
  *end_inout = my_end ;

  return ;
}


/* provide access to private macro, NB start is not used but defined by the macro */
int zmapFeatureRevCompCoord(int coord, int start, int end)
{
      return(zmapFeatureInvert(coord,start,end));
}




gboolean zmapDNA_strup(char *string, int length)
{
  gboolean good = TRUE;
  char up;

  while(length > 0 && good && string && *string)
    {
      switch(string[0])
        {
        case 'a':
          up = 'A';
          break;
        case 't':
          up = 'T';
          break;
        case 'g':
          up = 'G';
          break;
        case 'c':
          up = 'C';
          break;
        default:
          good = FALSE;
          break;
        }
      if(good)
        {
          string[0] = up;

          string++;
        }
      length--;
    }

  return good;
}




/* Extracts DNA for the given start/end from a block, doing a reverse complement if required.
 */
char *zMapFeatureGetDNA(ZMapFeatureAny feature_any, int start, int end, gboolean revcomp)
{
  char *dna = NULL ;

  if (zMapFeatureIsValid(feature_any) && (start > 0 && end >= start))
    {
      dna = getFeatureBlockDNA(feature_any, start, end, revcomp) ;
    }

  return dna ;
}


/* Trivial function which just uses the features start/end coords, could be more intelligent
 * and deal with transcripts/alignments more intelligently. */
char *zMapFeatureGetFeatureDNA(ZMapFeature feature)
{
  char *dna = NULL ;
  gboolean revcomp = FALSE ;

  if (zMapFeatureIsValid((ZMapFeatureAny)feature))
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	revcomp = TRUE ;

      dna = getFeatureBlockDNA((ZMapFeatureAny)feature, feature->x1, feature->x2, revcomp) ;
    }

  return dna ;
}


/* Get a transcripts DNA, this will probably mean extracting snipping out the dna for
 * each exon. */
char *zMapFeatureGetTranscriptDNA(ZMapFeature transcript, gboolean spliced, gboolean cds_only)
{
  char *dna = NULL ;

  if (zMapFeatureIsValidFull((ZMapFeatureAny)transcript, ZMAPFEATURE_STRUCT_FEATURE)
      && ZMAPFEATURE_IS_TRANSCRIPT(transcript))
    {
      char *tmp = NULL ;
      GArray *exons ;
      gboolean revcomp = FALSE ;
      ZMapFeatureBlock block = NULL ;
      int start = 0, end = 0 ;

      start = transcript->x1 ;
      end = transcript->x2 ;
      exons = transcript->feature.transcript.exons ;

      if (transcript->strand == ZMAPSTRAND_REVERSE)
	revcomp = TRUE ;

      if ((tmp = fetchBlockDNAPtr((ZMapFeatureAny)transcript, &block)) && coordsInBlock(block, &start, &end)
	  && (dna = getFeatureBlockDNA((ZMapFeatureAny)transcript, start, end, revcomp)))
	{
	  if (!spliced || !exons)
	    {
	      int i, offset, length;
	      gboolean upcase_exons = TRUE;

	      /* Paint the exons in uppercase. */
	      if (upcase_exons)
		{
		  if (exons)
		    {
		      int exon_start, exon_end ;

		      tmp = dna ;

		      for (i = 0 ; i < exons->len ; i++)
			{
			  ZMapSpan exon_span ;

			  exon_span = &(g_array_index(exons, ZMapSpanStruct, i)) ;

			  exon_start = exon_span->x1 ;
			  exon_end   = exon_span->x2 ;

			  if (zMapCoordsClamp(start, end, &exon_start, &exon_end))
			    {
			      offset  = dna - tmp ;
			      offset += exon_start - transcript->x1 ;
			      tmp    += offset ;
			      length  = exon_end - exon_start + 1 ;

			      zmapDNA_strup(tmp, length) ;
			    }
			}
		    }
		  else
		    {
		      zmapDNA_strup(dna, strlen(dna)) ;
		    }
		}
	    }
	  else
	    {
	      GString *dna_str ;
	      FeatureSeqFetcherStruct seq_fetcher = {NULL} ;

	      /* If cds is requested and transcript has one then adjust start/end for cds. */
	      if (!cds_only
		  || (cds_only && transcript->feature.transcript.flags.cds
		      && zMapCoordsClamp(transcript->feature.transcript.cds_start,
					 transcript->feature.transcript.cds_end, &start, &end)))
		{
		  int seq_length = 0 ;

		  seq_fetcher.dna_in  = tmp ;
		  seq_fetcher.dna_start = block->block_to_sequence.block.x1 ;
		  seq_fetcher.start = start ;
		  seq_fetcher.end = end ;

		  seq_fetcher.dna_out = dna_str = g_string_sized_new(1500) ; /* Average length of human proteins is
										apparently around 500 amino acids. */

		  zMapFeatureTranscriptExonForeach(transcript, fetch_exon_sequence, &seq_fetcher) ;

		  if (dna_str->len)
		    {
		      seq_length = dna_str->len ;
		      dna = g_string_free(dna_str, FALSE) ;

		      if (revcomp)
			zMapDNAReverseComplement(dna, seq_length) ;
		    }
		}
	    }
	}
    }

  return dna ;
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
	      zMapAssertNotReached();
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
                               ZMapFeatureStructType stop,
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
                                     ZMapFeatureStructType stop,
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
    zMapAssertNotReached();

  return ;
}

void zMapFeatureContextExecuteFull(ZMapFeatureAny feature_any,
                                   ZMapFeatureStructType stop,
                                   ZMapGDataRecurseFunc callback,
                                   gpointer data)
{
  feature_context_execute_full(feature_any, stop, callback,
			       NULL, FALSE, FALSE,
			       catch_hash_abuse_G, data);
  return ;
}

void zMapFeatureContextExecuteComplete(ZMapFeatureAny feature_any,
                                       ZMapFeatureStructType stop,
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
					 ZMapFeatureStructType stop,
					 ZMapGDataRecurseFunc start_callback,
					 ZMapGDataRecurseFunc end_callback,
					 gpointer data)
{
  feature_context_execute_full(feature_any, stop, start_callback,
			       end_callback, TRUE, FALSE,
			       catch_hash_abuse_G, data);
  return ;
}

void zMapFeatureContextExecuteStealSafe(ZMapFeatureAny feature_any,
					ZMapFeatureStructType stop,
					ZMapGDataRecurseFunc start_callback,
					ZMapGDataRecurseFunc end_callback,
					gpointer data)
{
  feature_context_execute_full(feature_any, stop, start_callback,
			       end_callback, FALSE, TRUE,
			       catch_hash_abuse_G, data);
  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapFeatureTypeStyle zMapFeatureContextFindStyle(ZMapFeatureContext context, char *style_name)
{
  ZMapFeatureTypeStyle style = NULL;

  style = zMapFindStyle(context->styles, zMapStyleCreateID(style_name));

  return style;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* This seems like an insane thing to ask for... Why would you want to
 * find a feature given a feature. Well the from feature will _not_ be
 * residing in the context, but must be in another context.
 */

ZMapFeatureAny zMapFeatureContextFindFeatureFromFeature(ZMapFeatureContext context,
							ZMapFeatureAny query_feature)
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
      feature_ptr = zMapFeatureGetParentGroup(feature_ptr, feature_ptr->struct_type - 1);
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
	  zMapAssertNotReached();
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
	  zMapAssertNotReached();
	  break;
	}

      if(current)
	feature_ptr = g_hash_table_lookup(feature_ptr->children,
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
 *                        Internal functions.
 */



/* If we can get the block and it has dna then return a pointer to the dna and optionally to the block. */
static char *fetchBlockDNAPtr(ZMapFeatureAny feature_any, ZMapFeatureBlock *block_out)
{
  char *dna = NULL ;
  ZMapFeatureBlock block = NULL;

  if ((block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature_any, ZMAPFEATURE_STRUCT_BLOCK))
      && block->sequence.sequence)
    {
      dna = block->sequence.sequence ;

      if (block_out)
	*block_out = block ;
    }

  return dna ;
}




static char *getFeatureBlockDNA(ZMapFeatureAny feature_any, int start_in, int end_in, gboolean revcomp)
{
  char *dna = NULL ;
  ZMapFeatureBlock block ;
  int start, end ;

  start = start_in ;
  end = end_in ;

  /* Can only get dna if there is dna for the block and the coords lie within the block. */
  if (fetchBlockDNAPtr(feature_any, &block) && coordsInBlock(block, &start, &end))
    {
      /* Transform block coords to 1-based for fetching sequence. */
      zMapFeature2BlockCoords(block, &start, &end) ;
      dna = getDNA(block->sequence.sequence, start, end, revcomp) ;
    }

  return dna ;
}



static char *getDNA(char *dna_sequence, int start, int end, gboolean revcomp)
{
  char *dna = NULL ;
  int length ;

  length = end - start + 1 ;

  dna = g_strndup((dna_sequence + start - 1), length) ;

  if (revcomp)
    zMapDNAReverseComplement(dna, length) ;

  return dna ;
}



/* THESE SHOULD BE IN UTILS.... */

/* Check whether coords overlap a block, returns TRUE for full or partial
 * overlap, FALSE otherwise.
 *
 * Coords returned in start_inout/end_inout are:
 *
 * no overlap           <start/end set to zero >
 * partial overlap      <start/end clipped to block>
 * complete overlap     <start/end unchanged>
 *
 *  */
static gboolean coordsInBlock(ZMapFeatureBlock block, int *start_inout, int *end_inout)
{
  gboolean result = FALSE ;

  if (!(result = zMapCoordsClamp(block->block_to_sequence.block.x1, block->block_to_sequence.block.x2,
				 start_inout, end_inout)))
    {
      *start_inout = *end_inout = 0 ;
    }
  return result ;
}


static ZMapFeatureContextExecuteStatus revCompFeaturesCB(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  RevCompData cb_data = (RevCompData)user_data;

  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;

            /* we revcomp a complete context so reflect in the parent span */

//zMapLogWarning("rev comp align 1, sequence span = %d -> %d", feature_align->sequence_span.x1,feature_align->sequence_span.x2);
        zmapFeatureRevComp(Coord, cb_data->start, cb_data->end,
                           feature_align->sequence_span.x1,
                           feature_align->sequence_span.x2) ;
//zMapLogWarning("rev comp align 2, sequence span = %d -> %d", feature_align->sequence_span.x1,feature_align->sequence_span.x2);

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

//zMapLogWarning("rev comp block 1, block = %d -> %d", //feature_block->block_to_sequence.block.x1,feature_block->block_to_sequence.block.x2);

        zmapFeatureRevComp(Coord, cb_data->start, cb_data->end,
                           feature_block->block_to_sequence.block.x1,
                           feature_block->block_to_sequence.block.x2) ;
        feature_block->revcomped = !feature_block->revcomped;

//zMapLogWarning("rev comp block 2, block = %d -> %d", //feature_block->block_to_sequence.block.x1,feature_block->block_to_sequence.block.x2);




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
//zMapLogWarning("rev comp set 1, span = %d -> %d",span->x1, span->x2);
            zmapFeatureRevComp(Coord, cb_data->start, cb_data->end, span->x1, span->x2) ;
//zMapLogWarning("rev comp set 2, span = %d -> %d",span->x1, span->x2);
	  }


	/* OK...THIS IS CRAZY....SHOULD BE PART OF THE FEATURE REVCOMP....FIX THIS.... */
	/* Now redo the 3 frame translations from the dna (if they exist). */
	if (feature_set->original_id == g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME))
	  zMapFeature3FrameTranslationSetRevComp(feature_set, cb_data->block_start, cb_data->block_end) ;


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
	zMapAssertNotReached();
	break;
      }
    }

  return status;
}

static void revcompSpan(GArray *spans, int seq_start, int seq_end)
{
  int i ;

  for (i = 0; i < spans->len; i++)
    {
      ZMapSpan span ;

      span = &g_array_index(spans, ZMapSpanStruct, i) ;

      zmapFeatureRevComp(Coord, seq_start, seq_end, span->x1, span->x2) ;
    }


  return ;
}


static void revCompFeature(ZMapFeature feature, int start_coord, int end_coord)
{
  zMapAssert(feature);

  zmapFeatureRevComp(Coord, start_coord, end_coord, feature->x1, feature->x2) ;

  if (feature->strand == ZMAPSTRAND_FORWARD)
    feature->strand = ZMAPSTRAND_REVERSE ;
  else if (feature->strand == ZMAPSTRAND_REVERSE)
    feature->strand = ZMAPSTRAND_FORWARD ;

  if (zMapFeatureSequenceIsPeptide(feature))
    {
      /* Original & Unique IDs need redoing as they include the frame which probably change on revcomp. */
      char *feature_name = NULL ;			    /* Remember to free this */
      GQuark feature_id ;

      /* Essentially this does nothing and needs removing... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMapFrame curr_frame ;

      curr_frame = zMapFeatureFrame(feature) ;
      feature->feature.sequence.frame = curr_frame ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      feature_name = zMapFeature3FrameTranslationFeatureName((ZMapFeatureSet)(feature->parent),
							     feature->feature.sequence.frame) ;
      feature_id = g_quark_from_string(feature_name) ;
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
  else if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT
	   && (feature->feature.transcript.exons || feature->feature.transcript.introns))
    {
      if (feature->feature.transcript.exons)
        revcompSpan(feature->feature.transcript.exons, start_coord, end_coord) ;

      if (feature->feature.transcript.introns)
        revcompSpan(feature->feature.transcript.introns, start_coord, end_coord) ;

      if(feature->feature.transcript.flags.cds)
        zmapFeatureRevComp(Coord, start_coord, end_coord,
                           feature->feature.transcript.cds_start,
                           feature->feature.transcript.cds_end);
    }
  else if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT
           && feature->feature.homol.align)
    {
      int i ;

      for (i = 0; i < feature->feature.homol.align->len; i++)
        {
          ZMapAlignBlock align ;

          align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;

          zmapFeatureRevComp(Coord, start_coord, end_coord, align->t1, align->t2) ;
#warning why does this not change t_strand?
        }

      zMapFeatureSortGaps(feature->feature.homol.align) ;
    }
  else if (feature->type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
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
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
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
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
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
      zMapAssertNotReached();
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
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  gboolean  remove_from_hash = FALSE;

  zMapAssert(zMapFeatureAnyHasMagic(feature_any));

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
			zMapAssertNotReached();
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
              zMapAssertNotReached();
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
      full_data->status ^= (full_data->status & ZMAP_CONTEXT_EXEC_NON_ERROR);	/* clear only non-error bits */
    }

  return remove_from_hash;
}


static void postExecuteProcess(ContextExecute execute_data)
{

  if(execute_data->status >= ZMAP_CONTEXT_EXEC_STATUS_ERROR)
    {
      zMapLogCritical("ContextExecute: Error, function stopped @ level %d, message = %s\n",
                      execute_data->stopped_at,
                      execute_data->error_string);
    }

  return ;
}

static void fetch_exon_sequence(gpointer exon_data, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon_data;
  FeatureSeqFetcher seq_fetcher = (FeatureSeqFetcher)user_data;
  int offset, length, start, end ;

  start = exon_span->x1;
  end = exon_span->x2;

  /* Check the exon lies within the  */
  if (zMapCoordsClamp(seq_fetcher->start, seq_fetcher->end, &start, &end))
    {
      offset = start - seq_fetcher->dna_start ;

      length = end - start + 1 ;

      seq_fetcher->dna_out = g_string_append_len(seq_fetcher->dna_out,
                                                 (seq_fetcher->dna_in + offset),
                                                 length) ;
    }

  return ;
}



gboolean zMapFeatureContextGetMasterAlignSpan(ZMapFeatureContext context,int *start,int *end)
{
      gboolean res = start && end;

      if(start) *start = context->master_align->sequence_span.x1;
      if(end)   *end   = context->master_align->sequence_span.x2;

      return(res);
}


static void feature_context_execute_full(ZMapFeatureAny feature_any,
					 ZMapFeatureStructType stop,
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

