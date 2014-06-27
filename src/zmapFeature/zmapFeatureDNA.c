/*  File: zmapFeatureDNA.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See <ZMap/zmapFeature.h>
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapDNA.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>



typedef struct FeatureSeqFetcherStructType
{
  /* Input dna string, note that start != 1 where we are looking at a sub-part of an assembly
   * but start/end must be inside the assembly start/end. */
  char *dna_in;
  int dna_start ;
  int start, end ;

  /* Output dna string. */
  GString *dna_out ;
} FeatureSeqFetcherStruct, *FeatureSeqFetcher ;


typedef struct BlockHasDNAStructType
{
  gboolean exists;
} BlockHasDNAStruct, *BlockHasDNA;





static gboolean hasFeatureBlockDNA(ZMapFeatureAny feature_any) ;
static char *getFeatureBlockDNA(ZMapFeatureAny feature_any, int start_in, int end_in, gboolean revcomp) ;
static void fetch_exon_sequence(gpointer exon_data, gpointer user_data);
static char *fetchBlockDNAPtr(ZMapFeatureAny feature, ZMapFeatureBlock *block_out) ;
static char *getDNA(char *dna, int start, int end, gboolean revcomp) ;
static gboolean coordsInBlock(ZMapFeatureBlock block, int *start_out, int *end_out) ;
static gboolean strupDNA(char *string, int length) ;



/* 
 *                    External routines
 */


gboolean zMapFeatureDNAExists(ZMapFeature feature)
{
  gboolean has_dna = FALSE ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  has_dna = hasFeatureBlockDNA((ZMapFeatureAny)feature) ;

  return has_dna ;
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


/* Get a transcripts DNA, this is done by getting the unspliced DNA and then snipping
 * out the exons, CDS.
 *
 * Returns unspliced/spliced/cds dna or NULL if there is no DNA in the feature context
 * or if CDS is requested and transcript does not have one or feature is not a transcript.
 *
 */
char *zMapFeatureGetTranscriptDNA(ZMapFeature transcript, gboolean spliced, gboolean cds_only)
{
  char *dna = NULL ;

  if (zMapFeatureIsValidFull((ZMapFeatureAny)transcript, ZMAPFEATURE_STRUCT_FEATURE)
      && ZMAPFEATURE_IS_TRANSCRIPT(transcript)
      && (!cds_only || (cds_only && transcript->feature.transcript.flags.cds)))
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
                        
                              strupDNA(tmp, length) ;
                            }
                        }
                    }
                  else
                    {
                      strupDNA(dna, strlen(dna)) ;
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



/*!
 * A Blocks DNA
 */
gboolean zMapFeatureBlockDNA(ZMapFeatureBlock block,
                             char **seq_name_out, int *seq_len_out, char **sequence_out)
{
  gboolean result = FALSE;
  ZMapFeatureContext context = NULL;

  if ( !block ) 
    return result ;

  if(block->sequence.sequence &&
     block->sequence.type != ZMAPSEQUENCE_NONE &&
     block->sequence.type == ZMAPSEQUENCE_DNA  &&
     (context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)block,
                                                              ZMAPFEATURE_STRUCT_CONTEXT)))
    {
      if(seq_name_out)
        *seq_name_out = (char *)g_quark_to_string(context->sequence_name) ;
      if(seq_len_out)
        *seq_len_out  = block->sequence.length ;
      if(sequence_out)
        *sequence_out = block->sequence.sequence ;
      result = TRUE ;
    }

  return result;
}

/* Free return when finished! */
char *zMapFeatureDNAFeatureName(ZMapFeatureBlock block)
{
  char *dna_name = NULL;

  dna_name = g_strdup_printf("%s (%s)", "DNA", g_quark_to_string(block->original_id));

  return dna_name;
}

/* sigh... canonicalise! */
GQuark zMapFeatureDNAFeatureID(ZMapFeatureBlock block)
{
  GQuark dna_id = 0 ;
  char *dna_name = NULL ;

  dna_name = zMapFeatureDNAFeatureName(block) ;

  if (dna_name)
    {
      dna_id = zMapFeatureCreateID(ZMAPSTYLE_MODE_SEQUENCE,
   dna_name, ZMAPSTRAND_FORWARD,
   block->block_to_sequence.block.x1,
   block->block_to_sequence.block.x2,
   0, 0) ;

      g_free(dna_name) ;
    }

  return dna_id ;
}


gboolean zMapFeatureDNACreateFeatureSet(ZMapFeatureBlock block, ZMapFeatureSet *feature_set_out)
{
  ZMapFeatureSet dna_feature_set = NULL;
  gboolean created = FALSE;
  GQuark dna_set_id = 0;

  dna_set_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

  if(!(dna_feature_set = zMapFeatureBlockGetSetByID(block, dna_set_id)))
    {
      GQuark original_id = 0;
      GQuark unique_id   = dna_set_id;

      original_id     = g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME);

      dna_feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, NULL) ;

      zMapFeatureBlockAddFeatureSet(block, dna_feature_set);

      created = TRUE;
    }

  if(feature_set_out)
    *feature_set_out = dna_feature_set;

  return created;
}

void zMapFeatureDNAAddSequenceData(ZMapFeature dna_feature, char *dna_str, int sequence_length)
{

  if (zMapFeatureSequenceIsDNA(dna_feature))
    {
      dna_feature->feature.sequence.sequence = dna_str;
      dna_feature->feature.sequence.length   = sequence_length;
      dna_feature->feature.sequence.type     = ZMAPSEQUENCE_DNA;
    }

  return ;
}

ZMapFeature zMapFeatureDNACreateFeature(ZMapFeatureBlock block, ZMapFeatureTypeStyle style,
                                        char *dna_str, int sequence_length)
{
  ZMapFeatureSet dna_feature_set = NULL;
  ZMapFeature dna_feature = NULL;
  GQuark dna_set_id = 0;

  zMapReturnValIfFail((block && dna_str && *dna_str && sequence_length), NULL) ;

  dna_set_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

  if ((dna_feature_set = zMapFeatureBlockGetSetByID(block, dna_set_id)))
    {
      char *feature_name, *sequence, *ontology;
      GQuark dna_id;
      int block_start, block_end;

      block_start = block->block_to_sequence.block.x1 ;
      block_end = block->block_to_sequence.block.x2 ;

      feature_name = zMapFeatureDNAFeatureName(block);

      dna_id = zMapFeatureDNAFeatureID(block);;

      if (block->sequence.sequence)
        {
          /* hmm, we've already got dna */

          /* We should be able to get the feature from the feature set, */
          /* but not before issuing a warning */
          zMapLogWarning("%s", "Block already has DNA");

          dna_feature = zMapFeatureSetGetFeatureByID(dna_feature_set, dna_id);
        }
      else
        {
          ZMapStrand strand = ZMAPSTRAND_FORWARD; /* DNA is forward */
        
          /* check dna length == block length? */
          sequence = ontology = NULL;
          ontology = "dna";
        
          dna_feature_set->style = style;
        
          dna_feature = zMapFeatureCreateFromStandardData(feature_name,
                                                          sequence,
                                                          ontology,
                                                          ZMAPSTYLE_MODE_SEQUENCE,
                                                          &dna_feature_set->style,
                                                          block_start,
                                                          block_end,
                                                          FALSE, 0.0,
                                                          strand) ;
        
          zMapFeatureSequenceSetType(dna_feature, ZMAPSEQUENCE_DNA) ;
          zMapFeatureDNAAddSequenceData(dna_feature, dna_str, sequence_length);

          zMapFeatureSetAddFeature(dna_feature_set, dna_feature);

          block->sequence.sequence = dna_feature->feature.sequence.sequence;
          block->sequence.type     = dna_feature->feature.sequence.type;
          block->sequence.length   = dna_feature->feature.sequence.length;

        }

      if(feature_name)
        g_free(feature_name);
    }


  return dna_feature;
}


/* Function to check whether any of the blocks has dna */
static ZMapFeatureContextExecuteStatus oneBlockHasDNA(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;

  BlockHasDNA dna = (BlockHasDNA)user_data;

  feature_type = feature_any->struct_type;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if(!dna->exists)
        dna->exists = (gboolean)(feature_block->sequence.length ? TRUE : FALSE);
      break;
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_FEATURESET:
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_ALIGN:
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapWarnIfReached();
      break;

    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}


gboolean zMapFeatureContextGetDNAStatus(ZMapFeatureContext context)
{
  gboolean drawable = FALSE;
  BlockHasDNAStruct dna = {0};

  /* We just need one of the blocks to have DNA.
   * This enables us to turn on this button as we
   * can't have half sensitivity.  Any block which
   * doesn't have DNA creates a warning for the user
   * to complain about.
   */

  if(context)
    {
      zMapFeatureContextExecute((ZMapFeatureAny)context,
                                ZMAPFEATURE_STRUCT_BLOCK,
                                oneBlockHasDNA,
                                &dna);

      drawable = dna.exists;
    }

  return drawable;
}





/* 
 *                      Internal functions
 */


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



static gboolean hasFeatureBlockDNA(ZMapFeatureAny feature_any)
{
  gboolean has_dna = FALSE ;
  ZMapFeatureBlock block ;

  if (fetchBlockDNAPtr(feature_any, &block))
    has_dna = TRUE ;

  return has_dna ;
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

/* should be in dna utils..... */
static gboolean strupDNA(char *string, int length)
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


