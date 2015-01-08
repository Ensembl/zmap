/*  File: zmapWindowSeqDisp.c
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
 * Description: Functions to manipulate canvas items.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapSequence.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowCanvasFeatureset_I.h>


static void highlightSequenceItems(ZMapWindow window, ZMapFeatureBlock block,
                                   FooCanvasItem *focus_item,
                                   ZMapSequenceType seq_type, ZMapFrame frame, int start, int end,
                                   gboolean centre_on_region, int flanking) ;

static void handleHightlightDNA(gboolean on, gboolean item_highlight, gboolean sub_feature,
                                ZMapWindow window, FooCanvasItem *item,
                                ZMapFrame required_frame,
                                ZMapSequenceType coords_type, int region_start, int region_end, int flanking) ;

static void highlightTranslationRegion(ZMapWindow window,
                                       gboolean highlight, gboolean item_highlight, gboolean sub_feature,
                                       FooCanvasItem *item,
                                       char *required_col, ZMapFrame required_frame,
                                       ZMapSequenceType coords_type, int region_start, int region_end, int flanking) ;

static void unHighlightTranslation(ZMapWindow window, FooCanvasItem *item,
                                   char *required_col, ZMapFrame required_frame) ;
static void handleHighlightTranslation(gboolean highlight,  gboolean item_highlight, gboolean sub_feature,
                                       ZMapWindow window, FooCanvasItem *item,
                                       char *required_col, ZMapFrame required_frame,
                                       ZMapSequenceType coords_type, int region_start, int region_end, int flanking) ;
static void handleHighlightTranslationSeq(gboolean highlight, gboolean item_highlight, gboolean sub_feature,
                                          FooCanvasItem *translation_item, FooCanvasItem *item,
                                          ZMapFrame required_frame,
                                          gboolean cds_only, ZMapSequenceType coords_type,
                                          int region_start, int region_end, int flanking ) ;
static FooCanvasItem *getTranslationItemFromItemFrame(ZMapWindow window,
                                                      ZMapFeatureBlock block, char *required_col, ZMapFrame frame) ;
static FooCanvasItem *translation_from_block_frame(ZMapWindow window, char *column_name,
                                                   gboolean require_visible,
                                                   ZMapFeatureBlock block, ZMapFrame frame) ;




/*
 *                        Package external functions.
 */


FooCanvasItem *zmapWindowItemGetDNATextItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *dna_item = NULL ;
  ZMapFeatureAny feature_any = NULL ;
  ZMapFeatureBlock block = NULL ;

  if ((feature_any = zmapWindowItemGetFeatureAny(item))
      && (block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature_any, ZMAPFEATURE_STRUCT_BLOCK)))
    {
      GQuark dna_set_id ;
      GQuark dna_id ;

      dna_set_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

      dna_id = zMapFeatureDNAFeatureID(block);


      if ((dna_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                                 block->parent->unique_id,
                                                 block->unique_id,
                                                 dna_set_id,
                                                 ZMAPSTRAND_FORWARD, /* STILL ALWAYS FORWARD */
                                                 /* since removing strand we seem to need this:
                                                  * the column is forward as the style is unstranded
                                                  * the featureset is reverse as the DNA feature has been revcomped
                                                  * the feature is forward ???? as the unique id was set on fwd strand ????
                                                  */
                                                 /* but as ST require fwds always it's better to keep the featureset fwds */
                                                 /* this is set in item factory run single() */
                                                 //                                                 window->flags[ZMAPFLAG_REVCOMPED_FEATURES]? ZMAPSTRAND_REVERSE : ZMAPSTRAND_FORWARD,

                                                 ZMAPFRAME_NONE, /* NO FRAME */
                                                 dna_id)))
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* this ought to work but it doesn't, I think this may because the parent (column) gets
           * unmapped but not the dna text item, so it's not visible any more but it is mapped ? */
          if(!(FOO_CANVAS_ITEM(dna_item)->object.flags & FOO_CANVAS_ITEM_VISIBLE))
            dna_item = NULL;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          if (!(dna_item->object.flags & FOO_CANVAS_ITEM_MAPPED))
            dna_item = NULL;
        }
    }

  return dna_item ;
}




/*
 * Sequence highlighting functions, these feel like they should be in the
 * sequence item class objects.
 */

/* this is from the mouse cursor only .... and DNAChoose */

void zmapWindowHighlightSequenceItem(ZMapWindow window, FooCanvasItem *item, int start, int end, int flanking)
{
  ZMapFeatureAny feature_any ;

  if ((feature_any = zmapWindowItemGetFeatureAny(FOO_CANVAS_ITEM(item))))
    {
      ZMapFeatureBlock block ;

      block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((feature_any), ZMAPFEATURE_STRUCT_BLOCK));

      highlightSequenceItems(window, block, NULL, ZMAPSEQUENCE_NONE, ZMAPFRAME_NONE, start, end, FALSE, flanking) ;
    }

  return ;
}







void zmapWindowHighlightSequenceRegion(ZMapWindow window, ZMapFeatureBlock block,
                                       ZMapSequenceType seq_type, ZMapFrame frame, int start, int end,
                                       gboolean centre_on_region, int flanking)
{
  highlightSequenceItems(window, block, NULL, seq_type, frame, start, end, centre_on_region, flanking) ;

  return ;
}



/* highlights the dna given any foocanvasitem (with a feature) and a start and end
 * This _only_ highlights in the current window! */
void zmapWindowItemHighlightDNARegion(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
                                      FooCanvasItem *item, ZMapFrame required_frame,
                                      ZMapSequenceType coords_type, int region_start, int region_end, int flanking)
{
  handleHightlightDNA(TRUE, item_highlight, sub_feature, window, item, required_frame,
                      coords_type, region_start, region_end, flanking) ;

  return ;
}


void zmapWindowItemUnHighlightDNA(ZMapWindow window, FooCanvasItem *item)
{
  handleHightlightDNA(FALSE, FALSE, FALSE, window, item, ZMAPFRAME_NONE, ZMAPSEQUENCE_NONE, 0, 0, 0) ;

  return ;
}



/* This function highlights the peptide translation columns from region_start to region_end
 * (in dna or pep coords). Note any existing highlight is removed. If required_frame
 * is set to ZMAPFRAME_NONE then highlighting is in all 3 cols otherwise only in the
 * frame column given. */
void zmapWindowItemHighlightTranslationRegions(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
                                               FooCanvasItem *item,
                                               ZMapFrame required_frame,
                                               ZMapSequenceType coords_type,
                                               int region_start, int region_end, int flanking)
{
  int frame ;

  for (frame = ZMAPFRAME_0 ; frame < ZMAPFRAME_2 + 1 ; frame++)
    {
      gboolean highlight ;

      if (required_frame == ZMAPFRAME_NONE || frame == required_frame)
        highlight = TRUE ;
      else
        highlight = FALSE ;

      highlightTranslationRegion(window,
                                 highlight, item_highlight, sub_feature,  item,
                                 ZMAP_FIXED_STYLE_3FT_NAME, frame,
                                 coords_type, region_start, region_end, flanking) ;
    }

  return ;
}


void zmapWindowItemUnHighlightTranslations(ZMapWindow window, FooCanvasItem *item)
{
  int frame ;

  for (frame = ZMAPFRAME_0 ; frame < ZMAPFRAME_2 + 1 ; frame++)
    {
      unHighlightTranslation(window, item, ZMAP_FIXED_STYLE_3FT_NAME, frame) ;
    }

  return ;
}


/* This function highlights the translation for a feature from region_start to region_end
 * (in dna or pep coords). Note any existing highlight is removed. */
void zmapWindowItemHighlightShowTranslationRegion(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
                                                  FooCanvasItem *item,
                                                  ZMapFrame required_frame,
                                                  ZMapSequenceType coords_type,
                                                  int region_start, int region_end, int flanking)
{
  highlightTranslationRegion(window, TRUE, item_highlight, sub_feature,
                             item, ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, required_frame,
                             coords_type, region_start, region_end, flanking) ;
  return ;
}


void zmapWindowItemUnHighlightShowTranslations(ZMapWindow window, FooCanvasItem *item)
{
  unHighlightTranslation(window, item, ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, ZMAPFRAME_NONE) ;

  return ;
}







/* chars for Show Translation column. */
#define SHOW_TRANS_BACKGROUND '='                            /* background char for entire column. */
#define SHOW_TRANS_INTRON     '-'                            /* char for introns. */

void zmapWindowItemShowTranslationRemove(ZMapWindow window, FooCanvasItem *feature_item)
{
  ZMapFeature feature;

  feature = zMapWindowCanvasItemGetFeature(feature_item);

  // remove is ok regardless of which feature we click on since displaying it
  //  if(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_FORWARD(feature))
  {
    ZMapFeatureAny feature_any;
    GQuark align_id, block_id;

    feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_ALIGN) ;
    align_id = feature_any->unique_id ;

    feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK) ;
    block_id = feature_any->unique_id ;

    /* Revist whether we need to do this call or just a redraw...... */
    zMapWindowToggleDNAProteinColumns(window, align_id, block_id, FALSE, FALSE, TRUE, FALSE, TRUE) ;
  }

  return ;
}

void zmapWindowItemShowTranslation(ZMapWindow window, FooCanvasItem *feature_to_translate)
{
  ZMapFeature feature ;

  feature = zMapWindowCanvasItemGetFeature(feature_to_translate);

  zmapWindowFeatureShowTranslation(window, feature) ;
}


void zmapWindowFeatureShowTranslation(ZMapWindow window, ZMapFeature feature)
{
  if (!(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_HAS_CDS(feature) && ZMAPFEATURE_FORWARD(feature)))
    {
      zMapWarning("%s %s",
                  zMapFeatureName((ZMapFeatureAny)feature),
                  (!ZMAPFEATURE_IS_TRANSCRIPT(feature) ? "is not a transcript.\n"
                   : (!ZMAPFEATURE_HAS_CDS(feature) ? "has no cds.\n"
                      : "must be on the forward strand.\n"))) ;
    }
  else
    {
      ZMapFeatureAny feature_any ;
      GList *trans_set ;
      ID2Canvas trans_id2c ;
      FooCanvasItem *trans_item ;
      ZMapFeatureBlock block ;
      ZMapFeature trans_feature ;
      GQuark wild_id ;
      GQuark align_id, block_id, set_id, feature_id ;
      gboolean force = TRUE, force_to = TRUE, do_dna = FALSE, do_aa = FALSE, do_trans = TRUE ;
      char *seq ;
      int len ;
      GList *exon_list_member;
      ZMapFullExon current_exon ;
      char *dest_ptr = NULL ;
      int dest_start = 0, dest_end = 0 ;
      double seq_start;
      ZMapWindowFeaturesetItem fset;
      gboolean found_cds = FALSE ;

      wild_id = zMapStyleCreateID("*") ;

      feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_ALIGN) ;
      align_id = feature_any->unique_id ;

      feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK) ;
      block = (ZMapFeatureBlock)feature_any ;
      block_id = feature_any->unique_id ;

      seq_start = block->block_to_sequence.block.x1;

      set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

      feature_id = wild_id ;

      trans_set = zmapWindowFToIFindItemSetFull(window, window->context_to_item,
                                                align_id, block_id, set_id,
                                                set_id,
                                                //                                                window->flags[ZMAPFLAG_REVCOMPED_FEATURES]? "-" : "+", ".",
                                                "+", ".",
                                                feature_id,
                                                NULL, NULL) ;

      /* trans set may once have been a column but now we don-t have columns in the ftoi hash
         context feature sets now refer to the canvasfeatureset foo item not the containing group
      */
      if(!trans_set)
        return;

      
      /* Remember the featureset we called show-translation for */
      if (feature && feature->parent)
        window->show_translation_featureset_id = feature->parent->unique_id ;

      trans_id2c = (ID2Canvas)(trans_set->data) ;

      trans_item = trans_id2c->item ;

      trans_feature = (ZMapFeature)trans_id2c->feature_any ;
      seq = trans_feature->feature.sequence.sequence ;
      len = trans_feature->feature.sequence.length ;

      fset = (ZMapWindowFeaturesetItem) trans_item;
      /* as we change viz status, cols to the right get exposed by full repos but this one does not move */
      //        zMapWindowCanvasFeaturesetExpose(fset);                /* so we get a double expose */

      /* Brute force, reinit the whole peptide string. */
      memset(seq, (int)SHOW_TRANS_BACKGROUND, trans_feature->feature.sequence.length) ;

      /* Destroy any previous exon/variation list */
      if (trans_feature->feature.sequence.exon_list)
        {
          g_list_free(trans_feature->feature.sequence.exon_list);
          trans_feature->feature.sequence.exon_list = NULL;
        }

      if (trans_feature->feature.sequence.variations)
        {
          g_list_free(trans_feature->feature.sequence.variations) ;
          trans_feature->feature.sequence.variations = NULL ;
        }
      
      /* If the feature contains any variations, store them because we'll need them to display
       * the translation correctly aligned. */
      if (feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        trans_feature->feature.sequence.variations = g_list_copy(feature->feature.transcript.variations) ;

      /* Get the exon descriptions from the feature. */
      zMapFeatureAnnotatedExonsCreate(feature, TRUE, FALSE, &trans_feature->feature.sequence.exon_list) ;

      /* Get first/last members and set background of whole transcript to '-' */
      GList *exon_list = trans_feature->feature.sequence.exon_list;
      exon_list_member = g_list_first(exon_list) ;
      do
        {
          current_exon = (ZMapFullExon)(exon_list_member->data) ;

          if (current_exon->region_type == EXON_CODING ||
              current_exon->region_type == EXON_SPLIT_CODON_5 ||
              current_exon->region_type == EXON_SPLIT_CODON_3)
            {
              dest_start = current_exon->sequence_span.x1 - seq_start + 1;
              found_cds = TRUE ; /* sanity check that a CDS exists */
              break ;
            }
        }
      while ((exon_list_member = g_list_next(exon_list_member))) ;

      if (!found_cds)
        {
          zMapWarning("%s %s", zMapFeatureName((ZMapFeatureAny)feature), "has no cds exons") ;
        }
      else
        {
          exon_list_member =  g_list_last(exon_list) ;
          do
            {
              current_exon = (ZMapFullExon)(exon_list_member->data) ;

              if (current_exon->region_type == EXON_CODING ||
                  current_exon->region_type == EXON_SPLIT_CODON_5 ||
                  current_exon->region_type == EXON_SPLIT_CODON_3)
                {
                  dest_end = current_exon->sequence_span.x2 - seq_start + 1;

                  break ;
                }
            }
          while ((exon_list_member = g_list_previous(exon_list_member))) ;


          if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(trans_item))
            zMapFeature2BlockCoords(block, &dest_start, &dest_end) ;

          dest_start = (dest_start + 2) / 3 ;        /* we assume frame 1, bias other frames backwards */
          dest_end = (dest_end + 2) / 3 ;

          memset(((seq + dest_start) - 1), (int)SHOW_TRANS_INTRON, ((dest_end - dest_start))) ;

          /* Now memcpy peptide strings into appropriate places, remember to divide seq positions
           * by 3 !!!!!! */
          exon_list_member = g_list_first(exon_list) ;
          do
            {
              gboolean show;

              current_exon = (ZMapFullExon)(exon_list_member->data) ;

              show = current_exon->region_type == EXON_CODING;
              if (current_exon->region_type == EXON_SPLIT_CODON_3 || current_exon->region_type == EXON_SPLIT_CODON_5)
                {
                  if(current_exon->sequence_span.x2 - current_exon->sequence_span.x1 > 0)
                    show = TRUE;
                }

              if(show)
                {
                  int tmp = 0 ;

                  dest_start = current_exon->sequence_span.x1 - seq_start + 1 ;

                  if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(trans_item))
                    zMapFeature2BlockCoords(block, &dest_start, &tmp) ;

                  dest_start = (dest_start + 2) / 3 ;

                  dest_ptr = (seq + dest_start) - 1 ;

                  /* Copy the src sequence to the dest sequence. */
                  memcpy(dest_ptr, current_exon->peptide, strlen(current_exon->peptide)) ;
                }
            }
          while ((exon_list_member = g_list_next(exon_list_member))) ;

      /* Revist whether we need to do this call or just a redraw...... */
      zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, do_trans, force_to, force) ;

#if 0
          /* i tried, but this does not highlight */
          /* but previous implementation didn't either */
          zmapWindowItemHighlightShowTranslationRegion(window, TRUE, FALSE,
                                                       feature_to_translate,
                                                       zMapFeatureFrame(feature),
                                                       ZMAPSEQUENCE_PEPTIDE,
                                                       feature->x1, feature->x2);
#endif
        }
    }

  return ;
}





/*
 *                  Internal routines.
 */



static void highlightSequenceItems(ZMapWindow window, ZMapFeatureBlock block,
                                   FooCanvasItem *focus_item,
                                   ZMapSequenceType seq_type, ZMapFrame required_frame, int start, int end,
                                   gboolean centre_on_region, int flanking)
{
  FooCanvasItem *item ;
  GQuark set_id ;
  ZMapFrame tmp_frame ;
  ZMapStrand tmp_strand ;
  gboolean done_centring = FALSE ;


  /* If there is a dna column then highlight match in that. */
  tmp_strand = ZMAPSTRAND_FORWARD ;        /* seq columns/ featruesets are always forward strand */
  tmp_frame = ZMAPFRAME_NONE ;
  set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;

  item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                    block->parent->unique_id, block->unique_id,
                                    set_id, tmp_strand, tmp_frame, 0);
  //printf("highlight sequence %s -> %p\n", g_quark_to_string(set_id), item);
  if (item)
    {
      int dna_start, dna_end ;

      dna_start = start ;
      dna_end = end ;

      zmapWindowItemHighlightDNARegion(window, FALSE, FALSE,  item, required_frame, seq_type, dna_start, dna_end, flanking) ;

      if (centre_on_region)
        {
          /* Need to convert sequence coords to block for this call. */
          zMapFeature2BlockCoords(block, &dna_start, &dna_end) ;


          zmapWindowItemCentreOnItemSubPart(window, item, FALSE, 0.0, dna_start, dna_end) ;
          done_centring = TRUE ;
        }
    }


  /* If there are peptide columns then highlight match in those. */
  tmp_strand = ZMAPSTRAND_FORWARD ;
  tmp_frame = ZMAPFRAME_NONE ;
  set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;

  {
    int frame_num, pep_start, pep_end ;

    for (frame_num = ZMAPFRAME_0 ; frame_num <= ZMAPFRAME_2 ; frame_num++)
      {
        pep_start = start ;
        pep_end = end ;

        if ((item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                               block->parent->unique_id, block->unique_id,
                                               set_id, tmp_strand, frame_num, 0)))
          {
            if (seq_type == ZMAPSEQUENCE_DNA)
              {
                highlightTranslationRegion(window, TRUE, FALSE, FALSE,
                                           item, ZMAP_FIXED_STYLE_3FT_NAME, frame_num, seq_type, pep_start, pep_end, flanking) ;
              }
            else
              {
#if HIGHLIGHT_CURSOR_FRAME
                // thinking if you cursor on one 3FT col you donlt highlight the others
                // but i doesn't work...
                if (required_frame == ZMAPFRAME_NONE || frame_num == required_frame)
                  highlightTranslationRegion(window, TRUE, FALSE, FALSE, item,
                                             ZMAP_FIXED_STYLE_3FT_NAME, frame_num, seq_type,
                                             pep_start, pep_end, flanking) ;
                else
                  unHighlightTranslation(window, item, ZMAP_FIXED_STYLE_3FT_NAME, frame_num) ;
#else
                highlightTranslationRegion(window, TRUE, FALSE, FALSE, item,
                                           ZMAP_FIXED_STYLE_3FT_NAME, frame_num, seq_type, pep_start, pep_end, flanking) ;
#endif
              }
          }
      }

    if (centre_on_region && !done_centring)
      zmapWindowItemCentreOnItemSubPart(window, item, FALSE, 0.0, start, end) ;
    done_centring = TRUE ;
  }

  set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

  if ((item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                         block->parent->unique_id, block->unique_id,
                                         set_id, tmp_strand, tmp_frame, 0)))
    {
      highlightTranslationRegion(window, TRUE, FALSE, FALSE,
                                 item, ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, required_frame, seq_type, start, end, flanking) ;

      if (centre_on_region && !done_centring)
        zmapWindowItemCentreOnItemSubPart(window, item, FALSE, 0.0, start, end) ;

    }


  return ;
}



gboolean zMapWindowSeqDispDeSelect(FooCanvasItem *sequence_feature)
{

  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(sequence_feature))
    {
      ZMapWindowCanvasItem canvas_item = ZMAP_CANVAS_ITEM(sequence_feature) ;
      ZMapFeature feature = zMapWindowCanvasItemGetFeature((FooCanvasItem * )canvas_item);
      zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, NULL,
                                             ZMAPSTYLE_COLOURTYPE_INVALID,  0,NULL,NULL);
    }
  return TRUE;
}





/* Highlight sequence in sequence_feature corresponding to seed_feature. */
gboolean zMapWindowSeqDispSelectByFeature(FooCanvasItem *sequence_feature,
                                          FooCanvasItem *item, ZMapFeature seed_feature,
                                          gboolean cds_only, gboolean sub_feature)
{
  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(sequence_feature))
    {
      ZMapFeature feature = zMapWindowCanvasItemGetFeature(sequence_feature);
      ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) sequence_feature;
      GdkColor *fill;
      ZMapFeatureSubPartSpanStruct span ;
      GQuark feature_style_id = 0 ;
      GQuark show_translation_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

      if (feature && feature->style && *feature->style)
        feature_style_id = (*feature->style)->unique_id ; 

      /*
       * previous code did deselect as part of the select operation
       * inside the TextItem inside the sequence feature
       */
      zMapWindowSeqDispDeSelect(sequence_feature) ;

      /* code copied from zmapWindowSequenceFeature.c and adjusted */

      /* default: simple features and sub-parts */

      switch(seed_feature->mode)
        {
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            GList *exon_list = NULL, *exon_list_member ;
            ZMapFullExon current_exon ;
            ZMapFrame frame = zMapFeatureFrame(feature);

            /* If no frame we just default to frame 1 which seems sensible. */
            if (!frame)
              frame = ZMAPFRAME_0 ;

            /* Get positions/translation etc of all exons. */
            if (!zMapFeatureAnnotatedExonsCreate(seed_feature, TRUE, TRUE, &exon_list))
              {
                zMapLogWarning("Could not find exons/introns in transcript %s", zMapFeatureName((ZMapFeatureAny) seed_feature)) ;
                break;
              }

            /*! \bug #warning single exon select is broken */
#if 0
            if (sub_feature)
              {
                /* Fix up this code in a minute...should be made into an exonlist as well
                 * with common code..... */
                int index;

                ZMapFeatureSubPartSpan exon_span;

                exon_span = zMapWindowCanvasItemIntervalGetData(item) ;
                if(!exon_span)
                  break;

                index = (exon_span->index - 1) / 2;                /* span index includes introns */
                current_exon = (ZMapFullExon)(g_list_nth_data(exon_list,index)) ;
                if(!current_exon)
                  break;

                span.start = current_exon->sequence_span.x1 ;
                span.end   = current_exon->sequence_span.x2 ;
                span.subpart = ZMAPFEATURE_SUBPART_MATCH;
                span.index = 0;

                zMapStyleGetColours(*feature->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED, &fill, NULL,NULL);

                zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                                       ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);
              }
            else
#endif
              {
                ZMapFeatureTypeStyle style ;
                /* most of these colours are just not used */
                ZMapStyleColourType colour_type = ZMAPSTYLE_COLOURTYPE_NORMAL ;
                GdkColor *non_coding_background = NULL, *non_coding_foreground = NULL, *non_coding_outline = NULL ;
                GdkColor *coding_background = NULL, *coding_foreground = NULL, *coding_outline = NULL ;
                GdkColor *split_codon_5_background = NULL, *split_codon_5_foreground = NULL, *split_codon_5_outline = NULL ;
                GdkColor *split_codon_3_background = NULL, *split_codon_3_foreground = NULL, *split_codon_3_outline = NULL ;
                GdkColor *in_frame_background = NULL, *in_frame_foreground = NULL, *in_frame_outline = NULL ;
                gboolean result ;
                int index = 1;
                gboolean is_pep = zMapFeatureSequenceIsPeptide(feature);
                gboolean in_frame = FALSE;

                style = *feature->style ;

                result = zMapStyleGetColours(style, STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, colour_type,
                                             &non_coding_background, &non_coding_foreground, &non_coding_outline) ;

                result = zMapStyleGetColours(style, STYLE_PROP_SEQUENCE_CODING_COLOURS, colour_type,
                                             &coding_background, &coding_foreground, &coding_outline) ;

                result = zMapStyleGetColours(style, STYLE_PROP_SEQUENCE_SPLIT_CODON_5_COLOURS, colour_type,
                                             &split_codon_5_background, &split_codon_5_foreground, &split_codon_5_outline) ;

                result = zMapStyleGetColours(style, STYLE_PROP_SEQUENCE_SPLIT_CODON_3_COLOURS, colour_type,
                                             &split_codon_3_background, &split_codon_3_foreground, &split_codon_3_outline) ;

                result = zMapStyleGetColours(style, STYLE_PROP_SEQUENCE_IN_FRAME_CODING_COLOURS, colour_type,
                                             &in_frame_background, &in_frame_foreground, &in_frame_outline) ;

                //zMapLogWarning("displaying frame %d",frame);


                for(exon_list_member = g_list_first(exon_list); exon_list_member ; exon_list_member = g_list_next(exon_list_member))
                  {
                    in_frame = FALSE;
                    int slice_coord;

                    current_exon = (ZMapFullExon)(exon_list_member->data) ;

                    span.start = current_exon->sequence_span.x1 ;
                    span.end = current_exon->sequence_span.x2 ;
                    span.index = index++;
                    span.subpart = ZMAPFEATURE_SUBPART_EXON;

                    //zMapLogWarning("exon %d, %d", span.start, span.end);

                    slice_coord = span.start - featureset->start;
                    /* 0 based. use featureset->start instead of feature->x1 in case we ever have two disjoint sequences, slice coords are visible seuqnece space at min zoom */

                    fill = non_coding_background;

                    switch (current_exon->region_type)
                      {
                      case EXON_NON_CODING:
                        fill = non_coding_background;
                        break ;

                      case EXON_CODING:
                        {
                          fill = coding_background ;
                          span.subpart = ZMAPFEATURE_SUBPART_EXON_CDS;

                          /* For peptides make in-frame column a different colour. */
                          if (is_pep)
                            {
                              if((slice_coord % 3) == (frame - ZMAPFRAME_0))
                                in_frame = TRUE;
                              //zMapLogWarning("coding  in_frame = %d  (%d %d)", in_frame, slice_coord, slice_coord % 3);

                              if(in_frame)
                                fill = in_frame_background ;
                            }
                          break ;
                        }

                        /* NOTE a 3' split codon is at the 3' end of an exon and 5'split codon at the 5' end of an exon  */

                      case EXON_SPLIT_CODON_3:
                        fill = split_codon_3_background ;
                        span.subpart = ZMAPFEATURE_SUBPART_SPLIT_3_CODON;
                        /* fall through */

                      case EXON_START_NOT_FOUND:        /* defaults to exon subpart */
                        if (is_pep)
                          {
                            if((slice_coord % 3) == (frame - ZMAPFRAME_0))
                              in_frame = TRUE;

                            /* For peptides only show split codon for the inframe column of the
                             * three-frame translation (confusing otherwise). However, for the
                             * show-translation column, always show it. */
                            if (!in_frame && feature_style_id != show_translation_id)
                              fill = coding_background ;
                          }
                        //zMapLogWarning("split3  in_frame = %d  (%d %d)", in_frame, slice_coord, slice_coord % 3);
                        break;

                      case EXON_SPLIT_CODON_5:
                        fill = split_codon_5_background ;
                        span.subpart = ZMAPFEATURE_SUBPART_SPLIT_5_CODON;

                        if (is_pep)
                          {
                            /* get phase from end of 2nd part of split codon, not the middle */
                            slice_coord = span.end + 1 - featureset->start;

                            if((slice_coord % 3) == (frame - ZMAPFRAME_0))
                              in_frame = TRUE;
                            //zMapLogWarning("split5  in_frame = %d  (%d %d)", in_frame, slice_coord, slice_coord % 3);

                            /* For peptides only show split codon for the inframe column of the
                             * three-frame translation (confusing otherwise). However, for the
                             * show-translation column, always show it. */
                            if (!in_frame && feature_style_id != show_translation_id)
                              fill = coding_background ;
                          }
                        break ;

                      default:
                        zMapWarnIfReached() ;
                      }

                    if (current_exon->region_type != EXON_NON_CODING
                        || (current_exon->region_type == EXON_NON_CODING && !cds_only))
                      {
                        zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                                               ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);
                      }
                  }
              }


            /* Tidy up. */
            zMapFeatureAnnotatedExonsDestroy(exon_list) ;

            break ;
          }

        default:                /* whole feature in red */
          {
            span.start = seed_feature->x1 ;
            span.end   = seed_feature->x2 ;
            span.subpart = ZMAPFEATURE_SUBPART_MATCH;
            span.index = 0;

            zMapStyleGetColours(*feature->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED, &fill, NULL,NULL);

            zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                                   ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);

          }
        }        /* end of switch */

    }
  return TRUE;
}



/* flanking is the number of bases at each side
 * an is only ever used fr4om DNAChoose
 */
gboolean zMapWindowSeqDispSelectByRegion(FooCanvasItem *sequence_feature,
                                         ZMapSequenceType coord_type, int region_start, int region_end, gboolean out_frame, int flanking)
{
  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(sequence_feature))
    {
      ZMapFeature feature = zMapWindowCanvasItemGetFeature(sequence_feature);
      GdkColor *fill;
      ZMapFeatureSubPartSpanStruct span ;

      /* previous code did this as part of the select operation
       * inside the TextItem inside the sequnece feature
       */
      zMapWindowSeqDispDeSelect(sequence_feature) ;

      if(flanking)
        {
          span.start = region_start - flanking ;
          span.end   = region_start - 1;
          span.index = 0;
          zMapStyleGetColours(*feature->style, STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                              &fill, NULL, NULL) ;

          zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                                 ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);
        }

      span.start = region_start ;
      span.end   = region_end;
      span.subpart = ZMAPFEATURE_SUBPART_MATCH;
      span.index = flanking ? 1 : 0;

      zMapStyleGetColours(*feature->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED, &fill, NULL,NULL);

      /* calling stack for this gets tangled up so it never triggers, needs restructuring */
      if(out_frame)
        zMapStyleGetColours(*feature->style, STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                            &fill, NULL, NULL) ;

      zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                             ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);

      if(flanking)
        {
          span.start = region_end + 1 ;
          span.end   = region_end + flanking;
          span.index = 2;
          zMapStyleGetColours(*feature->style, STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                              &fill, NULL, NULL) ;

          zMapWindowCanvasItemSetIntervalColours((FooCanvasItem *) sequence_feature,  feature, &span,
                                                 ZMAPSTYLE_COLOURTYPE_SELECTED,  0, fill ,NULL);
        }
    }
  return TRUE;
}


static void handleHightlightDNA(gboolean on, gboolean item_highlight, gboolean sub_feature,
                                ZMapWindow window, FooCanvasItem *item, ZMapFrame required_frame,
                                ZMapSequenceType coords_type, int region_start, int region_end, int flanking)
{
  FooCanvasItem *dna_item ;

  if ((dna_item = zmapWindowItemGetDNATextItem(window, item))
      &&
      ZMAP_IS_WINDOW_FEATURESET_ITEM (dna_item)
      //                && item != dna_item
      )
    {
      ZMapFeature feature ;

      /* highlight specially for transcripts (i.e. exons). */
      if (on)
        {
          if (item_highlight && (feature = zmapWindowItemGetFeature(item)))
            {
              zMapWindowSeqDispSelectByFeature(dna_item, item, feature, FALSE, sub_feature) ;
            }
          else
            {
              zMapWindowSeqDispSelectByRegion(dna_item, coords_type, region_start, region_end, FALSE, flanking) ;
            }
        }
      else
        {
          zMapWindowSeqDispDeSelect(dna_item) ;
        }
    }

  return ;
}


/* highlights the translation given any foocanvasitem (with a
 * feature), frame and a start and end (protein seq coords) */
/* This _only_ highlights in the current window! */
static void highlightTranslationRegion(ZMapWindow window,
                                       gboolean highlight, gboolean item_highlight, gboolean sub_feature, FooCanvasItem *item,
                                       char *required_col, ZMapFrame required_frame,
                                       ZMapSequenceType coords_type, int region_start, int region_end, int flanking)
{
  handleHighlightTranslation(highlight, item_highlight, sub_feature, window, item,
                             required_col, required_frame, coords_type, region_start, region_end, flanking) ;

  return ;
}


static void unHighlightTranslation(ZMapWindow window, FooCanvasItem *item,
                                   char *required_col, ZMapFrame required_frame)
{
  handleHighlightTranslation(FALSE, FALSE, FALSE,  window, item, required_col, required_frame, ZMAPSEQUENCE_NONE, 0, 0, 0) ;

  return ;
}


static void handleHighlightTranslation(gboolean highlight, gboolean item_highlight, gboolean sub_feature,
                                       ZMapWindow window, FooCanvasItem *item,
                                       char *required_col, ZMapFrame required_frame,
                                       ZMapSequenceType coords_type, int region_start, int region_end, int flanking)
{
  FooCanvasItem *translation_item = NULL;
  ZMapFeatureAny feature_any ;
  ZMapFeatureBlock block ;

  /*
    ok....here's the site of the crash...the feature returned by itemgetfeature looks valid but has no parent !!!
    therefore the call to get the block returns NULL and then the gettranslation call crashes....

    so there are quite a few problems to sort out here....why isn't the feature any valid.....where did it
    come from....UGH.....does malcolms code copy a feature ???? don't know.....
  */

  if ((feature_any = zmapWindowItemGetFeatureAny(item))
      && (block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)(feature_any), ZMAPFEATURE_STRUCT_BLOCK))))
    {
      if ((translation_item = getTranslationItemFromItemFrame(window, block, required_col, required_frame)))
        {
          gboolean cds_only = FALSE ;

          if (g_ascii_strcasecmp(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, required_col) == 0)
            cds_only = TRUE ;

          handleHighlightTranslationSeq(highlight, item_highlight, sub_feature,
                                        translation_item, item, required_frame,
                                        cds_only, coords_type, region_start, region_end, flanking) ;
        }
    }

  return ;
}


static void handleHighlightTranslationSeq(gboolean highlight, gboolean item_highlight, gboolean sub_feature,
                                          FooCanvasItem *translation_item, FooCanvasItem *item,
                                          ZMapFrame required_frame,
                                          gboolean cds_only, ZMapSequenceType coords_type,
                                          int region_start, int region_end, int flanking)
{
  ZMapFeature feature ;

  if (highlight)
    {
      if (item_highlight && (feature = zmapWindowItemGetFeature(item)))
        {
          zMapWindowSeqDispSelectByFeature(translation_item, item, feature, cds_only, sub_feature) ;
        }
      else
        {
          /* mh17: how odd, we start from highlighing a region and end up with a test here
           * depending on whether there's a feature... this could use some rationalistion
           */
          //          gboolean out_frame = (required_frame != ZMAPFRAME_NONE && real_frame != required_frame);

          zMapWindowSeqDispSelectByRegion(translation_item, coords_type, region_start, region_end, FALSE, flanking) ;
        }
    }
  else
    {
      zMapWindowSeqDispDeSelect( translation_item) ;
    }

  return ;
}


static FooCanvasItem *getTranslationItemFromItemFrame(ZMapWindow window,
                                                      ZMapFeatureBlock block,
                                                      char *req_col_name, ZMapFrame req_frame)
{
  FooCanvasItem *translation = NULL;

  /* Get the frame for the item... and its translation feature (ITEM_FEATURE_PARENT!) */
  translation = translation_from_block_frame(window, req_col_name, TRUE, block, req_frame) ;

  return translation ;
}


static FooCanvasItem *translation_from_block_frame(ZMapWindow window, char *column_name,
                                                   gboolean require_visible,
                                                   ZMapFeatureBlock block, ZMapFrame frame)
{
  FooCanvasItem *translation = NULL;
  GQuark feature_set_id, feature_id;
  ZMapFeatureSet feature_set;
  ZMapStrand strand = ZMAPSTRAND_FORWARD;

  feature_set_id = zMapStyleCreateID(column_name);
  /* and look up the translation feature set with ^^^ */

  if ((feature_set = zMapFeatureBlockGetSetByID(block, feature_set_id)))
    {
      char *feature_name;

      /* Get the name of the framed feature...and its quark id */
      if (g_ascii_strcasecmp(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, column_name) == 0)
        {
          GList *trans_set ;
          ID2Canvas trans_id2c ;

          feature_id = zMapStyleCreateID("*") ;

          trans_set = zmapWindowFToIFindItemSetFull(window, window->context_to_item,
                                                    block->parent->unique_id, block->unique_id, feature_set_id,
                                                    feature_set_id, "+", ".",
                                                    feature_id,
                                                    NULL, NULL) ;

          /* trans set may once have been a column but not we don't have columns in the ftoi hash
             context feature sets now refer to the canvasfeatureset foo item not the containing group
          */
          if(!trans_set)
            return NULL;

          trans_id2c = (ID2Canvas)(trans_set->data) ;

          translation = trans_id2c->item ;
        }
      else
        {
          feature_name = zMapFeature3FrameTranslationFeatureName(feature_set, frame) ;
          feature_id = g_quark_from_string(feature_name) ;

          translation = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                                   block->parent->unique_id,
                                                   block->unique_id,
                                                   feature_set_id,
                                                   strand, /* STILL ALWAYS FORWARD */
                                                   frame,
                                                   feature_id) ;

          //printf("translation: %p %s %s\n",translation, g_quark_to_string(feature_id), g_quark_to_string(feature_set_id));

          g_free(feature_name) ;
        }

      if (translation && require_visible && !(FOO_CANVAS_ITEM(translation)->object.flags & FOO_CANVAS_ITEM_VISIBLE))
        translation = NULL ;
    }

  return translation;
}



