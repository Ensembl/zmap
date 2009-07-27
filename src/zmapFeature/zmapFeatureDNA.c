/*  File: zmapFeatureDNA.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 15 09:04 2009 (rds)
 * Created: Tue Apr  7 10:32:21 2009 (rds)
 * CVS info:   $Id: zmapFeatureDNA.c,v 1.4 2009-07-27 03:16:20 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>


/*!
 * A Blocks DNA
 */
gboolean zMapFeatureBlockDNA(ZMapFeatureBlock block,
                             char **seq_name_out, int *seq_len_out, char **sequence_out)
{
  gboolean result = FALSE;
  ZMapFeatureContext context = NULL;

  zMapAssert( block ) ;

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
  GQuark dna_id = 0;
  char *dna_name = NULL;

  dna_name = zMapFeatureDNAFeatureName(block);

  if(dna_name)
    {
      dna_id = zMapFeatureCreateID(ZMAPSTYLE_MODE_RAW_SEQUENCE,
				   dna_name, ZMAPSTRAND_FORWARD,
				   block->block_to_sequence.q1,
				   block->block_to_sequence.q2, 
				   0, 0);
      g_free(dna_name);
    }

  return dna_id;
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

  if(!dna_feature->feature.sequence.sequence &&
     dna_feature->type == ZMAPSTYLE_MODE_RAW_SEQUENCE)
    {
      dna_feature->feature.sequence.sequence = dna_str;
      dna_feature->feature.sequence.length   = sequence_length;
      dna_feature->feature.sequence.type     = ZMAPSEQUENCE_DNA;
    }

  return ;
}

ZMapFeature zMapFeatureDNACreateFeature(ZMapFeatureBlock     block, 
					ZMapFeatureTypeStyle style,
					char *dna_str, 
					int   sequence_length)
{
  ZMapFeatureSet dna_feature_set = NULL;
  ZMapFeature dna_feature = NULL;
  GQuark dna_set_id = 0;

  zMapAssert(block);
  zMapAssert(dna_str);
  zMapAssert(sequence_length != 0);

  dna_set_id      = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

  dna_feature_set = zMapFeatureBlockGetSetByID(block, dna_set_id);

  if(dna_feature_set)
    {
      char *feature_name, *sequence, *ontology;
      GQuark dna_id;
      int block_start, block_end;
      
      block_start = block->block_to_sequence.q1 ;
      block_end   = block->block_to_sequence.q2 ;

      feature_name = zMapFeatureDNAFeatureName(block);

      dna_id       = zMapFeatureDNAFeatureID(block);;

      if(block->sequence.sequence)
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
	  dna_feature = zMapFeatureCreateFromStandardData(feature_name,
							  sequence,
							  ontology,
							  ZMAPSTYLE_MODE_RAW_SEQUENCE,
							  style,
							  block_start,
							  block_end,
							  FALSE, 0.0,
							  strand,
							  ZMAPPHASE_NONE);

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
