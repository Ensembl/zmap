/*  File: zmapWindowItemText.c
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
 * Description:
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapPeptide.h>
#include <zmapWindow_P.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemTextFillColumn.h>



/* THIS FILE SHOULD GO AND THE FUNCTIONS SHOULD BE INCORPORATED INTO OTHER FILES. THIS
 * USED TO CONTAIN CODE TO DO THE SHOW TRANSLATION COLUMN BUT THAT IS NOW DONE VIA
 * THE CODE IN THE items SUBDIRECTORY. */



/* NOTE.....code is character sensitive, if you use the '.' or ' ' chars then it doesn't
 * work...I don't know why...yet.....annoying..... */
/* chars for Show Translation column. */
#define SHOW_TRANS_BACKGROUND '='			    /* background char for entire column. */
#define SHOW_TRANS_INTRON     '-'			    /* char for introns. */



#if MH17_NOT_USED


/* This function translates the bounds of the first and last cells
 * into the bounds for a polygon to highlight both cells and all those
 * in between according to logical text flow.
 *
 * e.g. a polygon like
 *
 *       first cell
 *            |
 *           _V_________________
 * __________|AGCGGAGTGAGCTACGT|
 * |ACTACGACGGACAGCGAGCAGCGATTT|
 * |CGTTGCATTATATCCG__ACGATCG__|
 * |__CGATCGATCGTA__|
 *                 ^
 *                 |
 *             last cell
 *
 */
void zmapWindowItemTextOverlayFromCellBounds(FooCanvasPoints *overlay_points,
					     double          *first,
					     double          *last,
					     double           minx,
					     double           maxx)
{
  zMapAssert(overlay_points->num_points >= 8);

  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] > last[ITEMTEXT_CHAR_BOUND_TL_Y])
    {
      double *tmp;

      zMapLogWarning("Incorrect order of char bounds %f > %f",
                     first[ITEMTEXT_CHAR_BOUND_TL_Y],
                     last[ITEMTEXT_CHAR_BOUND_TL_Y]);
      tmp   = last;
      last  = first;
      first = tmp;
    }

  /* We use the first and last cell coords to
   * build the coords for the polygon.  We
   * also set defaults for the 4 widest x coords
   * as we can't know from the cell position how
   * wide the column is.  The exception to this
   * is dealt with below, for the case when we're
   * only overlaying across part or all of one
   * row, where we don't want to extend to the
   * edge of the column. */

  overlay_points->coords[0]    =
    overlay_points->coords[14] = first[ITEMTEXT_CHAR_BOUND_TL_X];
  overlay_points->coords[1]    =
    overlay_points->coords[3]  = first[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[2]    =
    overlay_points->coords[4]  = maxx;
  overlay_points->coords[5]    =
    overlay_points->coords[7]  = last[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[6]    =
    overlay_points->coords[8]  = last[ITEMTEXT_CHAR_BOUND_BR_X];
  overlay_points->coords[9]    =
    overlay_points->coords[11] = last[ITEMTEXT_CHAR_BOUND_BR_Y];
  overlay_points->coords[10]   =
    overlay_points->coords[12] = minx;
  overlay_points->coords[13]   =
    overlay_points->coords[15] = first[ITEMTEXT_CHAR_BOUND_BR_Y];

  /* Do some fixing of the default values if we're only on one row */
  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] == last[ITEMTEXT_CHAR_BOUND_TL_Y])
    overlay_points->coords[2]   =
      overlay_points->coords[4] = last[ITEMTEXT_CHAR_BOUND_BR_X];

  if(first[ITEMTEXT_CHAR_BOUND_BR_Y] == last[ITEMTEXT_CHAR_BOUND_BR_Y])
    overlay_points->coords[10]   =
      overlay_points->coords[12] = first[ITEMTEXT_CHAR_BOUND_TL_X];

  return ;
}


/* Possibly a Container call really... */
/* function to translate coords in points from text item coordinates to
 * overlay item coordinates. (goes via world). Why? Because the text item
 * moves in y axis so it is only ever in scroll region
 * (see foozmap-canvas-floating-group.c)
 */
void zmapWindowItemTextOverlayText2Overlay(FooCanvasItem   *item,
					   FooCanvasPoints *points)
{
  ZMapWindowContainerGroup container_parent;
  ZMapWindowContainerOverlay container_overlay;
  FooCanvasItem  *overlay_item;
  int i;

  for(i = 0; i < points->num_points * 2; i++, i++)
    {
      foo_canvas_item_i2w(item, &(points->coords[i]), &(points->coords[i+1]));
    }

  container_parent  = zmapWindowContainerCanvasItemGetContainer(item);
  container_overlay = zmapWindowContainerGetOverlay(container_parent);
  overlay_item      = FOO_CANVAS_ITEM(container_overlay);

  for(i = 0; i < points->num_points * 2; i++, i++)
    {
      foo_canvas_item_w2i(overlay_item, &(points->coords[i]), &(points->coords[i+1]));
    }

  return ;
}

void debug_text(char *text, int index)
{
  char *pep = text;
  printf("Text + %d\n", index);
  pep+= index;
  for(index = 0; index < 20; index++)
    {
      printf("%c", *pep);
      pep++;
    }
  printf("\n");
  return ;
}



/* Wrapper around foo_canvas_item_text_index2item()
 * ...
 */
gboolean zmapWindowItemTextIndex2Item(FooCanvasItem *item,
				      int index,
				      double *bounds)
{
  FooCanvasGroup *item_parent;
  ZMapFeature item_feature;
  gboolean index_found = FALSE;

  if ((item_feature = zMapWindowCanvasItemGetFeature(item)))
    {
      double ix1, iy1, ix2, iy2;
      item_parent = FOO_CANVAS_GROUP(item->parent);

      /* We need to get bounds for item_parent->ypos to be correct
       * Go figure. Je ne comprend pas */
      foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

      if (zMapFeatureSequenceIsDNA(item_feature))
	{
	  index      -= item_parent->ypos;
	  index_found = foo_canvas_item_text_index2item(item, index, bounds);
	}
      else if (zMapFeatureSequenceIsPeptide(item_feature))
	{
	  ZMapFrame frame;
	  int dindex, protein_start;
	  char *pep;

	  frame = zMapFeatureFrame(item_feature);

	  foo_canvas_item_i2w(item, &ix1, &iy1);

	  /* world to protein */
	  protein_start = (int)(index / 3);

	  pep = item_feature->description + protein_start;
#ifdef RDS_DONT_INCLUDE
	  printf("protein_start = %d\n", protein_start);
	  debug_text(item_feature->text, protein_start);
#endif

	  /* correct for where this text is drawn from. */
	  dindex         = (item_parent->ypos / 3) - 1; /* zero based */
#ifdef RDS_DONT_INCLUDE
	  debug_text(item_feature->text, dindex);
#endif
	  dindex         = (iy1 / 3) - 1; /* zero based */
#ifdef RDS_DONT_INCLUDE
	  debug_text(item_feature->text, dindex);
#endif
	  protein_start -= dindex;
#ifdef RDS_DONT_INCLUDE
	  printf("dindex = %d, protein_start = %d\n", dindex, protein_start);
#endif
	  index_found = foo_canvas_item_text_index2item(item, protein_start, bounds);
	}
    }
  return index_found;
}


#endif

void zmapWindowItemShowTranslationRemove(ZMapWindow window, FooCanvasItem *feature_item)
{
  FooCanvasItem *translation_column = NULL;
  ZMapFeature feature;

  feature = zMapWindowCanvasItemGetFeature(feature_item);

  if(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_FORWARD(feature))
    {
      /* get the column to draw it in, this involves possibly making it, so we can't do it in the execute call */
      if((translation_column = zmapWindowItemGetShowTranslationColumn(window, feature_item)))
	{
	  zmapWindowColumnSetState(window, FOO_CANVAS_GROUP(translation_column),
				   ZMAPSTYLE_COLDISPLAY_HIDE, TRUE);
	}
    }

  return ;
}



void zmapWindowItemShowTranslation(ZMapWindow window, FooCanvasItem *feature_to_translate)
{
  ZMapFeature feature ;

  feature = zMapWindowCanvasItemGetFeature(feature_to_translate);

  if (!(ZMAPFEATURE_IS_TRANSCRIPT(feature) && ZMAPFEATURE_HAS_CDS(feature) && ZMAPFEATURE_FORWARD(feature)))
    {
      zMapWarning("%s %s",
		  zMapFeatureName((ZMapFeatureAny)feature),
		  (!ZMAPFEATURE_IS_TRANSCRIPT(feature) ? "is not a transcript."
		   : (!ZMAPFEATURE_HAS_CDS(feature) ? "has no cds."
		      : "must be on the forward strand."))) ;
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
      GList *exon_list = NULL, *exon_list_member;
      ZMapFullExon current_exon ;
      char *pep_ptr ;
      int pep_start, pep_end ;


      wild_id = zMapStyleCreateID("*") ;

      feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_ALIGN) ;
      align_id = feature_any->unique_id ;

      feature_any = zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK) ;
      block = (ZMapFeatureBlock)feature_any ;
      block_id = feature_any->unique_id ;

      set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

      feature_id = wild_id ;

      trans_set = zmapWindowFToIFindItemSetFull(window, window->context_to_item,
						align_id, block_id, set_id,
						set_id, "+", ".",
						feature_id,
						NULL, NULL) ;

      trans_id2c = (ID2Canvas)(trans_set->data) ;

      trans_item = trans_id2c->item ;

      trans_feature = (ZMapFeature)trans_id2c->feature_any ;
      seq = trans_feature->feature.sequence.sequence ;
      len = trans_feature->feature.sequence.length ;


      /* Brute force, reinit the whole peptide string. */
      memset(seq, (int)SHOW_TRANS_BACKGROUND, trans_feature->feature.sequence.length) ;


      /* Get the exon descriptions from the feature. */
      zMapFeatureAnnotatedExonsCreate(feature, TRUE, &exon_list) ;

      /* Get first/last members and set background of whole transcript to '-' */
      exon_list_member = g_list_first(exon_list) ;
      do
	{
	  current_exon = (ZMapFullExon)(exon_list_member->data) ;

	  if (current_exon->region_type == EXON_CODING)
	    {
	      pep_start = current_exon->sequence_span.x1 ;
	      break ;
	    }
	}
      while ((exon_list_member = g_list_next(exon_list_member))) ;


      exon_list_member =  g_list_last(exon_list) ;
      do
	{
	  current_exon = (ZMapFullExon)(exon_list_member->data) ;

	  if (current_exon->region_type == EXON_CODING)
	    {
	      pep_end = current_exon->sequence_span.x2 ;

	      break ;
	    }
	}
      while ((exon_list_member = g_list_previous(exon_list_member))) ;


	if(!ZMAP_IS_WINDOW_CANVAS_FEATURESET_ITEM(trans_item))
	     zMapFeature2BlockCoords(block, &pep_start, &pep_end) ;

      pep_start = (pep_start + 2) / 3 ;	/* we assume frame 1, bias other frames backwards */
      pep_end = (pep_end + 2) / 3 ;

      memset(((seq + pep_start) - 1), (int)SHOW_TRANS_INTRON, ((pep_end - pep_start))) ;

      /* Now memcpy peptide strings into appropriate places, remember to divide seq positions
       * by 3 !!!!!! */
      exon_list_member = g_list_first(exon_list) ;
      do
	{
	  current_exon = (ZMapFullExon)(exon_list_member->data) ;

	  if (current_exon->region_type == EXON_CODING)
	    {
	      int tmp = 0 ;

	      pep_start = current_exon->sequence_span.x1 ;

		if(!ZMAP_IS_WINDOW_CANVAS_FEATURESET_ITEM(trans_item))
			zMapFeature2BlockCoords(block, &pep_start, &tmp) ;

		pep_start = (pep_start + 2) / 3 ;

	      pep_ptr = (seq + pep_start) - 1 ;
	      memcpy(pep_ptr, current_exon->peptide, strlen(current_exon->peptide)) ;
	    }
	}
      while ((exon_list_member = g_list_next(exon_list_member))) ;


	if(!ZMAP_IS_WINDOW_CANVAS_FEATURESET_ITEM(trans_item))
	{
		/* The redraw call needs to go into the func called by the g_object_set call.....check the
		* available foo_canvas calls........ */
		foo_canvas_item_request_redraw(trans_item) ;
		g_object_set(G_OBJECT(trans_item),
			PROP_TEXT_CHANGED_STR, TRUE,
			NULL) ;
	}

      /* Revist whether we need to do this call or just a redraw...... */
      zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, do_trans, force_to, force) ;

    }

  return ;
}




/*
 *                               INTERNALS
 */






