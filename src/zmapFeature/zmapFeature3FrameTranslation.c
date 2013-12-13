/*  File: zmapFeature3FrameTranslation.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: Functions supporting 3 frame translation objects derived
 *              dna sequence feature.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <zmapFeature_P.h>




static void destroySequenceData(ZMapFeature feature) ;
static gboolean feature3FrameTranslationPopulate(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style,
						 int block_start, int block_end) ;
static void translation_set_populate(ZMapFeatureBlock feature_block, ZMapFeatureSet feature_set,
				     ZMapFeatureTypeStyle style,
				     char *seq_name, char *seq, int block_start, int block_end) ;



static gboolean showTranslationPopulate(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style,
					int block_start, int block_end) ;
static void translationPopulate(ZMapFeatureBlock feature_block,
				ZMapFeatureSet feature_set,
				ZMapFeatureTypeStyle style,
				char *seq_name,
				char *dna,
				int block_start, int block_end) ;




/*
 *                External functions.
 */


gboolean zMapFeatureSequenceSetType(ZMapFeature feature, ZMapSequenceType type)
{
  gboolean result = FALSE ;

  if (feature->type == ZMAPSTYLE_MODE_SEQUENCE && !(feature->feature.sequence.type)
      && (type == ZMAPSEQUENCE_DNA || type == ZMAPSEQUENCE_PEPTIDE))
    {
      feature->feature.sequence.type = type ;
      result = TRUE ;
    }

  return result ;
}



gboolean zMapFeatureORFCreateSet(ZMapFeatureBlock block, ZMapFeatureSet *set_out)
{
  gboolean created = FALSE ;
  ZMapFeatureSet feature_set = NULL;
  GQuark frame_id = 0;

  /* No sequence. No Translation _return_ EARLY */
  if ((block->sequence.length))
    {
      frame_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_ORF_NAME);

      if (!(feature_set = zMapFeatureBlockGetSetByID(block, frame_id)))
	{
	  GQuark original_id = 0;
	  GQuark unique_id   = frame_id;

	  original_id = g_quark_from_string(ZMAP_FIXED_STYLE_ORF_NAME);

	  feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, NULL) ;

	  zMapFeatureBlockAddFeatureSet(block, feature_set);

	  created = TRUE;
	}

      if (set_out)
	*set_out = feature_set;
    }

  return created ;
}



gboolean zMapFeature3FrameTranslationCreateSet(ZMapFeatureBlock block, ZMapFeatureSet *set_out)
{
  gboolean created = FALSE ;
  ZMapFeatureSet feature_set = NULL;
  GQuark frame_id = 0;

  /* No sequence. No Translation _return_ EARLY */
  if ((block->sequence.length))
    {
      frame_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);

      if (!(feature_set = zMapFeatureBlockGetSetByID(block, frame_id)))
	{
	  GQuark original_id = 0;
	  GQuark unique_id   = frame_id;

	  original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME);

	  feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, NULL) ;

	  zMapFeatureBlockAddFeatureSet(block, feature_set);

	  created = TRUE;
	}

      if (set_out)
        *set_out = feature_set;
    }

  return created ;
}


void zMapFeatureORFSetCreateFeatures(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style, ZMapFeatureSet translation_fs)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;
  ZMapFrame frame = ZMAPFRAME_NONE;

  for (strand = ZMAPSTRAND_FORWARD ; strand <= ZMAPSTRAND_REVERSE; ++strand)
    {
      for (frame = ZMAPFRAME_0 ; frame <= ZMAPFRAME_2; ++frame)
        {
          ZMapFeature translation ;
          char *translation_name = NULL ;		/* Remember to free this */
          GQuark translation_id = 0 ;
          ZMapFrame curr_frame = ZMAPFRAME_NONE;
          

          curr_frame = frame ; //   = zMapFeatureFrameFromCoords(translation_block->block_to_sequence.block.x1, block_position) ;	/* ref to zMapFeatureFrame(): these are block relative frames */

          translation_name = zMapFeature3FrameTranslationFeatureName(translation_fs, curr_frame) ;
          translation_id   = g_quark_from_string(translation_name) ;

          if ((translation = zMapFeatureSetGetFeatureByID(translation_fs, translation_id)))
            {
              char *sequence = translation->feature.sequence.sequence;

              /* Loop through each peptide looking for stops */
              int i = 0;
              int prev = 0;
              
              for ( ; i < translation->feature.sequence.length; ++i)
                {
                  if (sequence[i] == '*')
                    {
                      if (i != prev)
                        {
                          /* Convert to dna index */
                          const int start = prev * 3;
                          const int end = ((i - 1) * 3) + 2;
                          
                          /* Create the ORF feature */
                          ZMapFeature orf_feature = zMapFeatureCreateEmpty() ;
                          char *feature_name = g_strdup_printf("ORF %d %d", start + 1, end + 1); /* display coords are 1-based */
                          
                          zMapFeatureAddStandardData(orf_feature, feature_name, feature_name,
                                                     NULL, NULL,
                                                     ZMAPSTYLE_MODE_BASIC, &feature_set->style,
                                                     start + translation->x1, end + translation->x1, FALSE, 0.0, strand);
                          
                          zMapFeatureSetAddFeature(feature_set, orf_feature);                          
                       }

                      prev = i + 1;
                    }
                }
            }
        }
    }
}


void zMapFeature3FrameTranslationSetCreateFeatures(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  /* public version of... */
  feature3FrameTranslationPopulate(feature_set, style, 0, 0) ;

  return ;
}


/* This function relies on the 3 frame translation features already existing....should make
 * sure this is true.......... */
void zMapFeature3FrameTranslationSetRevComp(ZMapFeatureSet feature_set, int block_start, int block_end)
{
  feature3FrameTranslationPopulate(feature_set, NULL, block_start, block_end) ;

  return ;
}


/* This function relies on the 3 frame translation features already existing....should make
 * sure this is true.......... */
void zMapFeatureORFSetRevComp(ZMapFeatureSet feature_set, ZMapFeatureSet translation_fs)
{
  zMapFeatureORFSetCreateFeatures(feature_set, NULL, translation_fs);
  
  return ;
}


char *zMapFeature3FrameTranslationFeatureName(ZMapFeatureSet feature_set, ZMapFrame frame)
{
  char *feature_name = NULL ;
  zMapReturnValIfFail(feature_set, feature_name) ;

  char *frame_str ;

  switch (frame)
    {
    case ZMAPFRAME_0:
      frame_str = "1" ;
      break ;
    case ZMAPFRAME_1:
      frame_str = "2" ;
      break ;
    case ZMAPFRAME_2:
      frame_str = "3" ;
      break ;
    default:
      frame_str = "." ;
    }

  feature_name = g_strdup_printf("%s_frame-%s", g_quark_to_string(feature_set->unique_id), frame_str);

  return feature_name;
}

void zMapFeature3FrameTranslationAddSequenceData(ZMapFeature feature, char *peptide_str, int sequence_length)
{

  if (zMapFeatureSequenceIsPeptide(feature) && !(feature->feature.sequence.sequence))
    {
      feature->feature.sequence.sequence = peptide_str ;
      feature->feature.sequence.length   = sequence_length ;
      feature->feature.sequence.type     = ZMAPSEQUENCE_PEPTIDE ;
    }

  return ;
}




/* Following functions are for "show translation", similar to 3-frame but a bit different. */


gboolean zMapFeatureShowTranslationCreateSet(ZMapFeatureBlock block, ZMapFeatureSet *set_out)
{
  gboolean created = FALSE ;
  ZMapFeatureSet feature_set = NULL;

  /* hmmmm, is this the way to test this...I don't think so....
   * No sequence, no Translation so _return_ EARLY */
  if ((block->sequence.length))
    {
      if (!(feature_set = zMapFeatureBlockGetSetByID(block,
						     zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME))))
	{
	  feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, NULL) ;

	  zMapFeatureBlockAddFeatureSet(block, feature_set);

	  created = TRUE ;
	}

      if (set_out)
	*set_out = feature_set ;
    }

  return created ;
}



void zMapFeatureShowTranslationSetCreateFeatures(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  /* public version of... */
  showTranslationPopulate(feature_set, style, 0, 0) ;

  return ;
}







/*
 *               internal routines
 */



/* Accepts NULL as style but the translation features must already
 * exist and we just revcomp the feature and hence do not need a style.
 *
 *  */
static gboolean feature3FrameTranslationPopulate(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style,
						 int block_start, int block_end)
{
  gboolean result = FALSE ;
  ZMapFeatureBlock feature_block;

  feature_block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set, ZMAPFEATURE_STRUCT_BLOCK) ;

  if ((feature_block->sequence.sequence))
    {
      char *sequence_name;

      sequence_name = (char *)g_quark_to_string(feature_block->original_id);

      translation_set_populate(feature_block,
			       feature_set,
			       style,
			       sequence_name,
			       feature_block->sequence.sequence,
			       block_start, block_end) ;

      result = TRUE ;
    }

  return result ;
}



static void translation_set_populate(ZMapFeatureBlock feature_block,
				     ZMapFeatureSet feature_set,
				     ZMapFeatureTypeStyle style,
				     char *seq_name,
				     char *dna,
				     int block_start, int block_end)
{
  int i, block_position ;

  block_position = feature_block->block_to_sequence.block.x1 ;     // actual loaded DNA not logical sequence start


  if (block_start == 0)
    {
      block_start = feature_block->block_to_sequence.block.x1 ;
      block_end = feature_block->block_to_sequence.block.x2 ;
    }


  for (i = ZMAPFRAME_0 ; dna && *dna && i <= ZMAPFRAME_2 ; i++, dna++, block_position++)
    {
      ZMapPeptide pep ;
      ZMapFeature translation ;
      char *feature_name = NULL ;		/* Remember to free this */
      GQuark feature_id ;
      ZMapFrame curr_frame ;
      char *peptide_str ;
      int peptide_length ;

	// curr_frame = (ZMapFrame) i;
      curr_frame   = zMapFeatureFrameFromCoords(feature_block->block_to_sequence.block.x1, block_position) ;	/* ref to zMapFeatureFrame(): these are block relative frames */

      feature_name = zMapFeature3FrameTranslationFeatureName(feature_set, curr_frame) ;
      feature_id   = g_quark_from_string(feature_name) ;

      pep = zMapPeptideCreateSafely(NULL, NULL, dna, NULL, FALSE) ;

      if ((translation = zMapFeatureSetGetFeatureByID(feature_set, feature_id)))
        {
	  int start, end, x1, x2 ;

	  destroySequenceData(translation) ;

	  /* Arcane....we need features to be shifted so they are revcompd before recomping
	   * otherwise the recomping puts them in the wrong position for showing the translation. */
	  start = block_start ;
	  end = block_end ;

	  x1 = (translation->x1 - start) ;
	  x1 = end - x1 ;

	  x2 = (translation->x2 - start) ;
	  x2 = end - x2 ;

	  translation->x1 = x2 ;
	  translation->x2 = x1 ;
	}
      else
        {
          int x1, x2 ;

	    x1 = block_position ;
          x2 = x1 + zMapPeptideFullSourceCodonLength(pep) - 1 ;

          translation = zMapFeatureCreateEmpty() ;
	    if(!feature_set->style)
			feature_set->style = style;
          zMapFeatureAddStandardData(translation, feature_name, feature_name,
                                     seq_name, "sequence",
                                     ZMAPSTYLE_MODE_SEQUENCE, &feature_set->style,
                                     x1, x2, FALSE, 0.0,
                                     ZMAPSTRAND_NONE) ;

	    zMapFeatureSequenceSetType(translation, ZMAPSEQUENCE_PEPTIDE) ;

          zMapFeatureSetAddFeature(feature_set, translation) ;
        }

      peptide_str = zMapPeptideSequence(pep) ;

      peptide_str = g_strdup(peptide_str) ;

      /* Get the peptide length in complete codons. */
      peptide_length = zMapPeptideFullCodonAALength(pep) ;

      zMapFeature3FrameTranslationAddSequenceData(translation, peptide_str, peptide_length) ;

      zMapPeptideDestroy(pep) ;

      if (feature_name)
	g_free(feature_name) ;
    }

  return ;
}


static void destroySequenceData(ZMapFeature feature)
{
  if (zMapFeatureSequenceIsPeptide(feature) && (feature->feature.sequence.sequence))
    {
      g_free(feature->feature.sequence.sequence) ;
      feature->feature.sequence.sequence = NULL ;
      feature->feature.sequence.length = 0 ;
    }

  return ;
}






/* "Show Translation" functions. */


static gboolean showTranslationPopulate(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style,
					int block_start, int block_end)
{
  gboolean result = FALSE ;
  ZMapFeatureBlock feature_block ;

  feature_block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set, ZMAPFEATURE_STRUCT_BLOCK) ;

  if ((feature_block->sequence.sequence))
    {
      char *sequence_name;

      sequence_name = (char *)g_quark_to_string(feature_block->original_id);

      translationPopulate(feature_block,
			  feature_set,
			  style,
			  sequence_name,
			  feature_block->sequence.sequence,
			  block_start, block_end) ;

      result = TRUE ;
    }

  return result ;
}



static void translationPopulate(ZMapFeatureBlock feature_block,
				ZMapFeatureSet feature_set,
				ZMapFeatureTypeStyle style,
				char *seq_name,
				char *dna,
				int block_start, int block_end)
{
  int block_position ;
  ZMapPeptide pep ;
  ZMapFeature translation ;
  char *feature_name = NULL ;			    /* Remember to free this */
  GQuark feature_id ;
  char *peptide_str ;
  int peptide_length ;
  int x1, x2 ;

  block_position = feature_block->block_to_sequence.block.x1 ;     // actual loaded DNA not logical sequence start


  if (block_start == 0)
    {
      block_start = feature_block->block_to_sequence.block.x1 ;
      block_end = feature_block->block_to_sequence.block.x2 ;
    }

  feature_name = ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME ;
  feature_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

  pep = zMapPeptideCreateEmpty(NULL, NULL, dna, FALSE) ;

  if ((translation = zMapFeatureSetGetFeatureByID(feature_set, feature_id)))
    {

      /* If there is one set we need to get rid of it.... */
      destroySequenceData(translation) ;

    }
  else
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      x1 = block_position ;
      x2 = x1 + zMapPeptideFullSourceCodonLength(pep) - 1 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      x1 = block_start ;
      x2 = block_end ;

	if(!feature_set->style)
		feature_set->style = style;

      translation = zMapFeatureCreateEmpty() ;

      zMapFeatureAddStandardData(translation, feature_name, feature_name,
				 seq_name, "sequence",
				 ZMAPSTYLE_MODE_SEQUENCE, &feature_set->style,
				 x1, x2, FALSE, 0.0,
				 ZMAPSTRAND_NONE) ;

      zMapFeatureSequenceSetType(translation, ZMAPSEQUENCE_PEPTIDE) ;

      zMapFeatureSetAddFeature(feature_set, translation) ;
    }


  peptide_str = zMapPeptideSequence(pep) ;

  /* Get the peptide length in complete codons....WHY....CHECK THIS..... */
  peptide_length = zMapPeptideFullCodonAALength(pep) ;

  peptide_str = g_malloc0(peptide_length + 1) ;

  memset(peptide_str, (int)'=', peptide_length) ;

  zMapFeature3FrameTranslationAddSequenceData(translation, peptide_str, peptide_length) ;

  zMapPeptideDestroy(pep) ;

  return ;
}







