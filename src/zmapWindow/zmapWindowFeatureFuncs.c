/*  Last edited: Jul 12 08:51 2011 (edgrif) */
/*  File: zmapWindowFeatureFuncs.c
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Sets of functions to do feature related operations
 *              such as running blixem to show alignments.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>


/* Call blixem for selected features, feature column or columns. Gets
 * called from menus and from keyboard short cut. If called from keyboard
 * short cut then  x_pos = ypos = 0.0.
 *
 * source param is for short reads data
 *  */
void zmapWindowCallBlixem(ZMapWindow window,
			  ZMapWindowAlignSetType requested_homol_set,
			  char *source,
			  double x_pos, double y_pos)
{
  FooCanvasItem *focus_item = NULL ;
  ZMapFeatureAny feature_any ;


  if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
    focus_item = zmapWindowItemGetTrueItem(focus_item) ;

  if (!focus_item && requested_homol_set != ZMAPWINDOW_ALIGNCMD_FEATURES)
    focus_item = FOO_CANVAS_ITEM(zmapWindowFocusGetHotColumn(window->focus)) ;


  /* Test there was an item selected otherwise we don't know which aligns to show. */
  if (!focus_item)
    {
      zMapWarning("%s", "Could not launch blixem, no features selected.") ;
    }
  else if ((feature_any = zmapWindowItemGetFeatureAnyType(focus_item, -1)))
    {
      int y1, y2, window_start, window_end ;
      FooCanvasGroup *block_grp ;
      ZMapFeature feature = NULL ;
      gboolean found_window = FALSE ;
      gboolean selected_features = FALSE ;

#if MH17_FIXING_THE_BODGE_INSTEAD_OF_ADDING_ANOTHER
      if (!(found_window = zMapWindowGetVisibleSeq(window, focus_item, &window_start, &window_end)))
	{
	/* this function already contains a bodge for an unexpected failure */
	/* sad really, get visible seq fails because world2seq fails, so let's call world2seq again and hope it works */

	  /* y_pos not set so set to middle of visible window (don't care about x_pos).  */
	  if (x_pos == 0.0 && y_pos == 0)
	    y_pos = (window_start + window_end) / 2 ;

	  found_window = zmapWindowWorld2SeqCoords(window, x_pos, y_pos, x_pos, y_pos, &block_grp, &y1, &y2) ;
	}
#else
     	/* so we fix it by passing the foo iten we know we have */
	found_window = zMapWindowGetVisibleSeq(window, focus_item, &window_start, &window_end);
#endif

      if (!found_window)
	{
	  char *msg = "Failed to find mouse and/or window position" ;

	  zMapLogCritical("%s", msg) ;

	  zMapMessage("%s", msg) ;
	}
      else
	{
	  switch(feature_any->struct_type)
	    {
	    case ZMAPFEATURE_STRUCT_FEATURESET:
	      {
		/* Use first feature in set to get alignment type etc. */
		/* MH17: but you can have empty columns ... */
		feature = zMap_g_hash_table_nth(((ZMapFeatureSet)feature_any)->features, 0) ;

		break ;
	      }
	    case ZMAPFEATURE_STRUCT_FEATURE:
	      {
		feature = (ZMapFeature)feature_any ;

		/* User clicked on an alignment feature. */
		if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
		  {
		    selected_features = TRUE ;
		  }

		break ;
	      }
	    default:
	      {
		break;
	      }
	    }


	  if (feature)
	    {
	      ZMapWindowCallbackCommandAlign align ;
	      ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;

	      align = g_new0(ZMapWindowCallbackCommandAlignStruct, 1) ;

	      /* Set up general command field for callback. */
	      align->cmd = ZMAPWINDOW_CMD_SHOWALIGN ;

	      /* may be null if (temporary) blixem BAM option selected */
	      if (feature)
		align->block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
									   ZMAPFEATURE_STRUCT_BLOCK) ;
	      else
		align->block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature_any,
									   ZMAPFEATURE_STRUCT_BLOCK) ;
	      zMapAssert(align->block) ;


	      /* Set up all the parameters blixem needs to do it's display. */
	      align->offset = window->sequence->start ;

	 	zmapWindowWorld2SeqCoords(window, focus_item, 0, y_pos, 0,0, NULL, &y1,NULL) ;

	      align->cursor_position = y1 ;

	      align->window_start = window_start ;
	      align->window_end = window_end ;

	      if (zmapWindowMarkIsSet(window->mark))
		zmapWindowMarkGetSequenceRange(window->mark, &(align->mark_start), &(align->mark_end)) ;

	      if (requested_homol_set == ZMAPWINDOW_ALIGNCMD_SEQ)
		{
		  align->homol_type = ZMAPHOMOL_N_HOMOL ;

		  align->source = source ;
		  align->homol_set = requested_homol_set ;
		}
	      else if (feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
		{
		  /* User may click on non-homol feature if they want to see some other feature + dna in blixem. */
		  align->homol_type = ZMAPHOMOL_N_HOMOL ;

		  align->homol_set = ZMAPWINDOW_ALIGNCMD_NONE ;
		}
	      else
		{
		  align->homol_type = feature->feature.homol.type ;

		  align->feature_set = (ZMapFeatureSet)(feature->parent) ;

		  /* If user clicked on features then make a list of them (may only be one), otherwise
		   * we need to use the feature set. */
		  if (selected_features == TRUE)
		    {
		      GList *focus_items ;

		      focus_items = zmapWindowFocusGetFocusItemsType(window->focus, WINDOW_FOCUS_GROUP_FOCUS) ;

		      align->features = zmapWindowItemListToFeatureList(focus_items) ;

		      g_list_free(focus_items) ;

		      /* I DON'T LIKE THIS BIT....SEE IF WE CAN NOT DO THIS....LEAVE NULL
		       * IF NO EXPLICIT FEATURE CLICKED ON.... */
		      /* If no highlighted features then use the one the user clicked on. */
		      if (!(align->features))
			align->features = g_list_append(align->features, feature) ;
		    }

		  align->homol_set = requested_homol_set ;
		}

	      (*(window_cbs_G->command))(window, window->app_data, align) ;
	    }
	}
    }


  return ;
}

