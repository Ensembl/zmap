/*  File: zmapFeatureContext.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions that operate on the feature context such as
 *              for reverse complementing.
 *              
 * Exported functions: See ZMap/zmapFeature.h
 * HISTORY:
 * Last edited: Apr 28 20:05 2008 (rds)
 * Created: Tue Jan 17 16:13:12 2006 (edgrif)
 * CVS info:   $Id: zmapFeatureContext.c,v 1.36 2008-04-28 19:11:21 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>

/* This isn't ideal, but there is code calling the getDNA functions
 * below, not checking return value... */
#define NO_DNA_STRING "<No DNA>"

#define ZMAP_CONTEXT_EXEC_NON_ERROR (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE | ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND)
#define ZMAP_CONTEXT_EXEC_RECURSE_OK (ZMAP_CONTEXT_EXEC_STATUS_OK | ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE)
#define ZMAP_CONTEXT_EXEC_RECURSE_FAIL (~ ZMAP_CONTEXT_EXEC_RECURSE_OK)

typedef struct
{
  int end ;
} RevCompDataStruct, *RevCompData ;

typedef struct
{
  GString *dna_out;
  char *dna_in;
  int cds_start, cds_end;
  int seq_length;
  gboolean cds_only, has_cds;
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

static char *getDNA(char *dna, int start, int end, gboolean revcomp) ;
static void revcompDNA(char *sequence, int seq_length) ;
static void revCompFeature(ZMapFeature feature, int end_coord);
static ZMapFeatureContextExecuteStatus revCompFeaturesCB(GQuark key, 
                                                         gpointer data, 
                                                         gpointer user_data,
                                                         char **error_out);
static void revcompSpan(GArray *spans, int seq_end) ;
static gboolean fetchBlockDNAPtr(ZMapFeatureAny feature, char **dna);

static gboolean executeDataForeachFunc(gpointer key, gpointer data, gpointer user_data);
static void fetch_exon_sequence(gpointer exon_data, gpointer user_data);
static void postExecuteProcess(ContextExecute execute_data);
static gboolean nextIsQuoted(char **text) ;
static void copyQuarkCB(gpointer data, gpointer user_data) ;

static gboolean catch_hash_abuse_G = TRUE;

/* Reverse complement a feature context.
 * 
 * Efficiency does not allow us to simply throw everything away and reconstruct the context.
 * Therefore we have to go through and carefully reverse everything.
 *  */
void zMapFeatureReverseComplement(ZMapFeatureContext context)
{

  /*	features need to have their positions reversed and also their strand.
    what about blocks ??? they also need doing... and in fact there are the alignment
    * mappings etc.....needs some thought and effort.... */

  RevCompDataStruct cb_data ;

  /* Get this first... */
  cb_data.end = context->sequence_to_parent.c2 ;
  /* Then swop this... */
  zmapFeatureSwop(Coord, context->sequence_to_parent.p1, context->sequence_to_parent.p2) ;

  /* Because this doesn't allow for execution at context level ;( */
  zMapFeatureContextExecute((ZMapFeatureAny)context, 
                            ZMAPFEATURE_STRUCT_FEATURE,
                            revCompFeaturesCB,
                            &cb_data);

  return ;
}




/* Extracts DNA for the given start/end from a block, doing a reverse complement if required.
 */
char *zMapFeatureGetDNA(ZMapFeatureAny feature_any, int start, int end, gboolean revcomp)
{
  char *dna = NULL, *tmp;

  zMapAssert(zMapFeatureIsValid(feature_any)) ;

  if (fetchBlockDNAPtr(feature_any, &tmp))
    dna = getDNA(tmp, start, end, revcomp) ;

  return dna ;
}


/* Trivial function which just uses the features start/end coords, could be more intelligent
 * and deal with transcripts/alignments more intelligently. */
char *zMapFeatureGetFeatureDNA(ZMapFeatureContext context, ZMapFeature feature)
{
  char *dna = NULL, *tmp;
  gboolean revcomp = FALSE ;

  /* should check that feature is in context.... */
  zMapAssert(feature) ;

  if (feature->strand == ZMAPSTRAND_REVERSE)
    revcomp = TRUE ;

  if(fetchBlockDNAPtr((ZMapFeatureAny)feature, &tmp))
    dna = getDNA(tmp, feature->x1, feature->x2, revcomp) ;
  else if(NO_DNA_STRING)
    dna = g_strdup(NO_DNA_STRING);

  return dna ;
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


/* Get a transcripts DNA, this will probably mean extracting snipping out the dna for
 * each exon. */
char *zMapFeatureGetTranscriptDNA(ZMapFeatureContext context, ZMapFeature transcript,
				  gboolean spliced, gboolean cds_only)
{
  char *dna = NULL, *tmp;
  GArray *exons ;
  
  /* should check that feature is in context.... */
  zMapAssert(context && transcript && transcript->type == ZMAPSTYLE_MODE_TRANSCRIPT) ;

  exons = transcript->feature.transcript.exons ;

  if(fetchBlockDNAPtr((ZMapFeatureAny)transcript, &tmp))
    {
      gboolean revcomp = FALSE;

      if(transcript->strand == ZMAPSTRAND_REVERSE)
        revcomp = TRUE;

      if (!spliced || !exons)
        {
          int i, start, end, offset, length;
          gboolean upcase_exons = TRUE;
          
          dna = getDNA(tmp, transcript->x1, transcript->x2, revcomp);
          
          if(upcase_exons)
            {
              if(exons)
                {
                  tmp = dna;
                  
                  for(i = 0 ; i < exons->len ; i++)
                    {
                      ZMapSpan exon_span;
                      
                      exon_span = &(g_array_index(exons, ZMapSpanStruct, i));
                      
                      start = exon_span->x1;
                      end   = exon_span->x2;
                      
                      offset  = dna - tmp;
                      offset += start - transcript->x1;
                      tmp    += offset;
                      length  = end - start + 1;
                      
                      zmapDNA_strup(tmp, length);
                    }
                }
              else
                zmapDNA_strup(dna, strlen(dna));
            }
        }
      else
        {
          GString *dna_str ;
          int cds_start, cds_end ;
          int seq_length = 0 ;
          gboolean has_cds = FALSE;
          FeatureSeqFetcherStruct seq_fetcher = {NULL};

          seq_fetcher.dna_in  = tmp;
          seq_fetcher.dna_out = dna_str = g_string_sized_new(1500) ;		    /* Average length of human proteins is
                                                                                       apparently around 500 amino acids. */
          if(cds_only && (has_cds = transcript->feature.transcript.flags.cds))
            {
              cds_start = transcript->feature.transcript.cds_start;
              cds_end   = transcript->feature.transcript.cds_end;
            }

          seq_fetcher.cds_only   = cds_only;
          seq_fetcher.has_cds    = has_cds;
          seq_fetcher.seq_length = seq_length;
          seq_fetcher.cds_start  = cds_start;
          seq_fetcher.cds_end    = cds_end;
                    
          zMapFeatureTranscriptExonForeach(transcript, fetch_exon_sequence, &seq_fetcher);

          seq_length = seq_fetcher.seq_length;

          dna = g_string_free(dna_str, FALSE) ;
          
          if (revcomp)
            revcompDNA(dna, seq_length) ;
        }
    }

  if(!dna && NO_DNA_STRING)
    dna = g_strdup(NO_DNA_STRING);

  return dna ;
}




/* Take a string containing space separated context names (e.g. perhaps a list
 * of featuresets: "coding fgenes codon") and convert it to a list of
 * proper context id quarks. */
GList *zMapFeatureString2QuarkList(char *string_list)
{
  GList *context_quark_list = NULL ;
  char *list_pos = NULL ;
  char *next_context = NULL ;
  char *target ;

  target = string_list ;
  do
    {
      GQuark context_id ;

      if (next_context)
	{
	  target = NULL ;
	  context_id = zMapStyleCreateID(next_context) ;
	  context_quark_list = g_list_append(context_quark_list, GUINT_TO_POINTER(context_id)) ;
	}
      else
	list_pos = target ;
    }
  while (((list_pos && nextIsQuoted(&list_pos))
	  && (next_context = strtok_r(target, "\"", &list_pos)))
	 || (next_context = strtok_r(target, " \t", &list_pos))) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMap_g_quark_list_print(context_quark_list) ;	    /* debug.... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return context_quark_list ;
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

ZMapFeatureTypeStyle zMapFeatureContextFindStyle(ZMapFeatureContext context, char *style_name)
{
  ZMapFeatureTypeStyle style = NULL;

  style = zMapFindStyle(context->styles, zMapStyleCreateID(style_name));

  return style;
}

/* 
 *                        Internal functions. 
 */


static char *getDNA(char *dna_sequence, int start, int end, gboolean revcomp)
{
  char *dna = NULL ;
  int length ;

  zMapAssert(dna_sequence && (start > 0 && end >= start)) ;

  length = end - start + 1 ;

  dna = g_strndup((dna_sequence + start - 1), length) ;

  if (revcomp)
    revcompDNA(dna, length) ;

  return dna ;
}

static gboolean fetchBlockDNAPtr(ZMapFeatureAny feature_any, char **dna)
{
  gboolean dna_exists = FALSE;
  ZMapFeatureBlock block = NULL;

  if((block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature_any, ZMAPFEATURE_STRUCT_BLOCK))
     && block->sequence.sequence)
    {
      if(dna && (dna_exists = TRUE))
        *dna = block->sequence.sequence;
    }

  return dna_exists;
}

/* Reverse complement the DNA. This function is fast enough for now, if it proves too slow
 * then rewrite it !
 * 
 * It works by starting at each end and swopping the bases and then complementing the bases
 * so that the whole thing is done in place.
 *  */
static void revcompDNA(char *sequence, int length)
{
  char *s, *e ;
  int i ;

  for (s = sequence, e = (sequence + length - 1), i = 0 ;
       i < length / 2 ;
       s++, e--, i++)
    {
      char tmp ;

      tmp = *s ;
      *s = *e ;
      *e = tmp ;

      switch (*s)
	{
	case 'a':
	  *s = 't' ;
	  break ;
	case 't':
	  *s = 'a' ;
	  break ;
	case 'c':
	  *s = 'g' ;
	  break ;
	case 'g':
	  *s = 'c' ;
	  break ;
	}

      switch (*e)
	{
	case 'a':
	  *e = 't' ;
	  break ;
	case 't':
	  *e = 'a' ;
	  break ;
	case 'c':
	  *e = 'g' ;
	  break ;
	case 'g':
	  *e = 'c' ;
	  break ;
	}
    }

  return ;
}

static void revCompFeature(ZMapFeature feature, int end_coord)
{
  zMapAssert(feature);

  zmapFeatureRevComp(Coord, end_coord, feature->x1, feature->x2) ;
  
  if (feature->strand == ZMAPSTRAND_FORWARD)
    feature->strand = ZMAPSTRAND_REVERSE ;
  else if (feature->strand == ZMAPSTRAND_REVERSE)
    feature->strand = ZMAPSTRAND_FORWARD ;
  
  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT
      && (feature->feature.transcript.exons || feature->feature.transcript.introns))
    {
      if (feature->feature.transcript.exons)
        revcompSpan(feature->feature.transcript.exons, end_coord) ;
      
      if (feature->feature.transcript.introns)
        revcompSpan(feature->feature.transcript.introns, end_coord) ;
      
      if(feature->feature.transcript.flags.cds)
        zmapFeatureRevComp(Coord, end_coord, 
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
          
          zmapFeatureRevComp(Coord, end_coord, align->t1, align->t2) ;
        }

      zMapFeatureSortGaps(feature->feature.homol.align) ;
    }
  

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

  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;
  
  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align = NULL;
        feature_align = (ZMapFeatureAlignment)feature_any;
        /* Nothing to swop? */
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapFeatureBlock feature_block = NULL;
	ZMapFeatureSet translations ;
        GQuark translation_id;
        
        feature_block  = (ZMapFeatureBlock)feature_any;
        translation_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);

	/* Complement the dna. */
        if (feature_block->sequence.sequence)
          {
            revcompDNA(feature_block->sequence.sequence, feature_block->sequence.length) ;

            /* Now redo the 3 frame translations from the dna (if they exist). */
            if ((translations = zMapFeatureBlockGetSetByID(feature_block, translation_id)))
              zMapFeature3FrameTranslationRevComp(translations, cb_data->end);
          }
        else
          {
            /* paranoia makes me want to check we haven't got a three frame translation... */
            if ((translations = zMapFeatureBlockGetSetByID(feature_block, translation_id)))
              zMapFeatureSetDestroy(translations, TRUE);
          }

        zmapFeatureRevComp(Coord, cb_data->end,
                           feature_block->block_to_sequence.t1, 
                           feature_block->block_to_sequence.t2) ;
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = NULL;
        feature_set = (ZMapFeatureSet)feature_any;
        /* Nothing to swop here, I think..... */
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature_ft = NULL;

        feature_ft = (ZMapFeature)feature_any;

        revCompFeature(feature_ft, cb_data->end);
      }
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapAssertNotReached();
      break;

    }

  return status;
}

static void revcompSpan(GArray *spans, int seq_end)
{
  int i ;

  for (i = 0; i < spans->len; i++)
    {
      ZMapSpan span ;
      
      span = &g_array_index(spans, ZMapSpanStruct, i) ;

      zmapFeatureRevComp(Coord, seq_end, span->x1, span->x2) ;
    }
  

  return ;
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
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;

  YourDataType  all_data = (YourDataType)user_data;
  
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
  ZMapFeatureContext feature_context = NULL;
  ZMapFeatureAlignment feature_align = NULL;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeature          feature_ft    = NULL;
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
              feature_ft    = (ZMapFeature)feature_any;
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
      full_data->status ^= ZMAP_CONTEXT_EXEC_NON_ERROR;	/* clear only non-error bits */
    }
    
  return remove_from_hash;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This function will be completely useless when blocks are GData */
static void executeListForeachFunc(gpointer data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureBlock block = NULL;

  if(feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK)
    {
      block = (ZMapFeatureBlock)feature_any;
      executeDataForeachFunc(block->unique_id, (gpointer)block, user_data);
    }
  else
    zMapAssertNotReached();

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
  gboolean ignore = FALSE;
              
  start = exon_span->x1;
  end   = exon_span->x2;
              
  if(seq_fetcher->cds_only && seq_fetcher->has_cds)
    {
      /* prune out exons not within cds boundaries  */
      if(((seq_fetcher->cds_start > end) || (seq_fetcher->cds_end < start)))
        ignore = TRUE;		/* So we just return... */
      else
	{
	  /* Check if start or end needs setting to a coord (cds_start
	   * or cds_end) within _this_ exon */
	  if((seq_fetcher->cds_start >= start) && (seq_fetcher->cds_start <= end))
	    start = seq_fetcher->cds_start;
	  if((seq_fetcher->cds_end >= start) && (seq_fetcher->cds_end <= end))
	    end   = seq_fetcher->cds_end;
	}
    }
              
  if(!ignore)
    {
      offset = start - 1 ;
      length = end - start + 1 ;
      seq_fetcher->seq_length += length ;
      
      seq_fetcher->dna_out = g_string_append_len(seq_fetcher->dna_out, 
                                                 (seq_fetcher->dna_in + offset), 
                                                 length) ;
    }

  return ;
}


/* Look through string to see if next non-space char is a quote mark, if it is return TRUE
 * and set *text to point to the quote mark, otherwise return FALSE and leave *text unchanged. */
static gboolean nextIsQuoted(char **text)
{
  gboolean quoted = FALSE ;
  char *text_ptr = *text ;

  while (*text_ptr)
    {
      if (!(g_ascii_isspace(*text_ptr)))
	{
	  if (*text_ptr == '"')
	    {
	      quoted = TRUE ;
	      *text = text_ptr ;
	    }
	  break ;
	}

      text_ptr++ ;
    }

  return quoted ;
}




