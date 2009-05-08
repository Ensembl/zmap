/*  File: zmapFeature3FrameTranslation.c
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
 * Last edited: May  1 19:01 2009 (rds)
 * Created: Wed Apr  8 16:18:11 2009 (rds)
 * CVS info:   $Id: zmapFeature3FrameTranslation.c,v 1.1 2009-05-08 14:19:54 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <zmapFeature_P.h>


void zmapFeature3FrameTranslationDestroySequenceData(ZMapFeature feature);


static void zmapFeature3FrameTranslationPopulate(ZMapFeatureSet       feature_set, 
						 ZMapFeatureTypeStyle style);
static void fudge_rev_comp_translation(gpointer key, gpointer value, gpointer user_data);
static void translation_set_populate(ZMapFeatureBlock     feature_block,
				     ZMapFeatureSet       feature_set, 
				     ZMapFeatureTypeStyle style,
				     char *seq_name, 
				     char *seq);

gboolean zMapFeature3FrameTranslationCreateSet(ZMapFeatureBlock block, ZMapFeatureSet *set_out) 
{
  ZMapFeatureSet feature_set = NULL;
  GQuark frame_id = 0;
  gboolean created = FALSE;

  /* No sequence. No Translation _return_ EARLY */
  if(!(block->sequence.length))
    return created;

  frame_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);

  if(!(feature_set = zMapFeatureBlockGetSetByID(block, frame_id)))
    {
      GQuark original_id = 0;
      GQuark unique_id   = frame_id;

      original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME);

      feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, NULL) ;

      zMapFeatureBlockAddFeatureSet(block, feature_set);

      created = TRUE;
    }

  if(set_out)
    *set_out = feature_set;

  return created;
}

void zMapFeature3FrameTranslationSetCreateFeatures(ZMapFeatureSet feature_set,
						   ZMapFeatureTypeStyle style)
{
  /* public version of... */
  zmapFeature3FrameTranslationPopulate(feature_set, style);

  return ;
}

void zMapFeature3FrameTranslationSetRevComp(ZMapFeatureSet feature_set, int origin)
{
  zmapFeature3FrameTranslationPopulate(feature_set, NULL);

  /* We have to do this as the features get rev comped later, but
   * we're actually recreating the translation in the new orientation
   * so the numbers don't need rev comping then, so we do it here. 
   * I figured doing it twice was less hassle than special case 
   * elsewhere... RDS */
  g_hash_table_foreach(feature_set->features, fudge_rev_comp_translation, GINT_TO_POINTER(origin));

  return ;
}

char *zMapFeature3FrameTranslationFeatureName(ZMapFeatureSet feature_set, ZMapFrame frame)
{
  char *feature_name = NULL, *frame_str;

  switch (frame)
    {
    case ZMAPFRAME_0:
      frame_str = "0" ;
      break ;
    case ZMAPFRAME_1:
      frame_str = "1" ;
      break ;
    case ZMAPFRAME_2:
      frame_str = "2" ;
      break ;
    default:
      frame_str = "." ;
    }

  feature_name = g_strdup_printf("%s_frame-%s", g_quark_to_string(feature_set->unique_id), frame_str);

  return feature_name;
}

void zMapFeature3FrameTranslationAddSequenceData(ZMapFeature feature, char *peptide_str, int sequence_length)
{
  
  if(!feature->feature.sequence.sequence &&
     feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      feature->feature.sequence.sequence = peptide_str;
      feature->feature.sequence.length   = sequence_length;
      feature->feature.sequence.type     = ZMAPSEQUENCE_PEPTIDE;
    }
  
  return ;
}



void zmapFeature3FrameTranslationDestroySequenceData(ZMapFeature feature)
{
  if(feature->feature.sequence.sequence != NULL &&
     feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      g_free(feature->feature.sequence.sequence);
      feature->feature.sequence.sequence = NULL;
      feature->feature.sequence.length   = 0;
      feature->feature.sequence.type     = ZMAPSEQUENCE_NONE;
    }

  return ;
}

/* INTERNALS */

/* Accepts NULL as style. */
static void zmapFeature3FrameTranslationPopulate(ZMapFeatureSet       feature_set, 
						 ZMapFeatureTypeStyle style)
{
  ZMapFeatureTypeStyle temp_style = NULL;
  ZMapFeatureBlock feature_block;
  char *sequence_name;
  char *dna_sequence;

  feature_block = 
    (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set,
						ZMAPFEATURE_STRUCT_BLOCK);

  zMapAssert(feature_block);

  sequence_name = (char *)g_quark_to_string(feature_block->original_id);

  dna_sequence  = feature_block->sequence.sequence;

  if(style == NULL)
    {
      temp_style = style = zMapStyleCreate(ZMAP_FIXED_STYLE_3FT_NAME, 
					   ZMAP_FIXED_STYLE_3FT_NAME_TEXT);
    }

  if(dna_sequence)
    translation_set_populate(feature_block,
			     feature_set, 
			     style,
			     sequence_name,
			     dna_sequence);
  
  if(temp_style)
    zMapStyleDestroy(temp_style);

  return ;
}

static void fudge_rev_comp_translation(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)value;
  zmapFeatureRevComp(Coord, GPOINTER_TO_INT(user_data), feature->x1, feature->x2);
  return ;
}

static void translation_set_populate(ZMapFeatureBlock     feature_block,
				     ZMapFeatureSet       feature_set, 
				     ZMapFeatureTypeStyle style, 
				     char *seq_name, 
				     char *dna)
{
  int i, block_position;
  ZMapFeature frame_feature;
  char *feature_name_id = "__delete_me__";
  char *feature_name    = "__delete_me__";
  char *sequence        = "__delete_me__";
  char *ontology        = "sequence";

  frame_feature = zMapFeatureCreateEmpty();

  zMapFeatureAddStandardData(frame_feature, feature_name_id,
			     feature_name, sequence,
			     ontology, ZMAPSTYLE_MODE_PEP_SEQUENCE, 
                             style, 1, 10, FALSE, 0.0, 
			     ZMAPSTRAND_NONE, ZMAPPHASE_NONE);

  zMapFeatureSetAddFeature(feature_set, frame_feature);

  block_position = feature_block->block_to_sequence.q1;

  for (i = ZMAPFRAME_0; dna && *dna && i <= ZMAPFRAME_2; i++, dna++, block_position++)
    {
      ZMapPeptide pep;
      ZMapFeature translation;
      char *feature_name = NULL; /* Remember to free this */
      GQuark feature_id;
      ZMapFrame curr_frame;
      char *peptide_str;
      int peptide_length;

      frame_feature->x1 = block_position;

      curr_frame   = zMapFeatureFrame(frame_feature);
      feature_name = zMapFeature3FrameTranslationFeatureName(feature_set, curr_frame);
      feature_id   = g_quark_from_string(feature_name);
      
      pep = zMapPeptideCreateSafely(NULL, NULL, dna, NULL, FALSE);
      
      if((translation = zMapFeatureSetGetFeatureByID(feature_set, feature_id)))
        {
	  /* clear sequence? */
	  zmapFeature3FrameTranslationDestroySequenceData(translation);
	}
      else
        {
          int x1, x2;

          x1 = frame_feature->x1;
          x2 = x1 + zMapPeptideFullSourceCodonLength(pep) - 1;

          translation = zMapFeatureCreateEmpty();
          
          zMapFeatureAddStandardData(translation, feature_name, feature_name,
                                     seq_name, "sequence",
                                     ZMAPSTYLE_MODE_PEP_SEQUENCE, style,
                                     x1, x2, FALSE, 0.0,
                                     ZMAPSTRAND_NONE, ZMAPPHASE_NONE);

          zMapFeatureSetAddFeature(feature_set, translation);
        }

      peptide_str    = zMapPeptideSequence(pep);

      peptide_str    = g_strdup(peptide_str);

      peptide_length = zMapPeptideLength(pep);

      zMapFeature3FrameTranslationAddSequenceData(translation, peptide_str, peptide_length);
      
      zMapPeptideDestroy(pep) ;

      if(feature_name)
	g_free(feature_name);
    }

  zMapFeatureSetRemoveFeature(feature_set, frame_feature);
  zMapFeatureDestroy(frame_feature);

  return ;
}
