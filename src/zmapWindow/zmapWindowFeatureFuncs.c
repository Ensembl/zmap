/*  Last edited: Jul 12 08:51 2011 (edgrif) */
/*  File: zmapWindowFeatureFuncs.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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


/* Call blixem for selected features, feature column or columns.
 *
 * Gets called from menus and from keyboard short cut. If called from keyboard
 * short cut then  x_pos = ypos = 0.0.
 *
 * Note user can click menu on a column that is not selected in which case
 * item will be non-NULL but focus item/column may be NULL if no other column
 * has been selected.
 *
 * source param is for short reads data
 *  */
void zmapWindowCallBlixem(ZMapWindow window, FooCanvasItem *item,
			  ZMapWindowAlignSetType requested_homol_set,
			  ZMapFeatureSet feature_set, GList *source,
			  double x_pos, double y_pos)
{
  FooCanvasItem *focus_item = NULL, *focus_column = NULL ;
  int window_start, window_end ;
  gboolean item_in_focus_col = FALSE ;
  gboolean selected_features = FALSE ;
  ZMapFeatureAny feature_any ;


  /* Get focus item/column if there is one. */
  if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
    focus_item = zmapWindowItemGetTrueItem(focus_item) ;

  focus_column = FOO_CANVAS_ITEM(zmapWindowFocusGetHotColumn(window->focus)) ;


  /* If we are passed an item then see if it's in the focus column and see if there are selected
   * features in that column. */
  if (item)
    {
      ZMapContainerLevelType level ;
      FooCanvasItem *item_column = item ;

      level = zmapWindowContainerUtilsGetLevel(item) ;

      if (level != ZMAPCONTAINER_LEVEL_FEATURESET)
	item_column = (FooCanvasItem *)zmapWindowContainerUtilsItemGetParentLevel(item,
										  ZMAPCONTAINER_LEVEL_FEATURESET) ;

      if (item_column == focus_column)
	{
	  item_in_focus_col = TRUE ;
	  selected_features = TRUE ;
	}
    }


  if (!item && (!focus_item || !focus_column))
    {
      /* If there was no selected item and there are no focus items we can't do anything. */

      zMapWarning("Could not launch blixem, %s is selected !", (!focus_column ? "nothing" : "no feature")) ;
    }
  else if (!zMapWindowGetVisibleSeq(window, (item ? item : focus_item), &window_start, &window_end))
    {
      /* If we can't find where we are in the window we can't do anything. */
      char *msg = "Failed to find mouse and/or window position" ;

      zMapLogCritical("%s", msg) ;

      zMapMessage("%s", msg) ;
    }
  else if ((requested_homol_set == ZMAPWINDOW_ALIGNCMD_FEATURES || requested_homol_set == ZMAPWINDOW_ALIGNCMD_EXPANDED)
		&& !(item_in_focus_col && focus_item))
    {
      /* User wants to blixem "selected" features but there aren't any in the item's column. */

      zMapWarning("%s", "Could not launch blixem, no selected features in this column.") ;
    }
  else if (!(feature_any = zmapWindowItemGetFeatureAnyType(item, -1))
	   || (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURESET
	       && feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
	   || (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	       && !(feature_any = zMap_g_hash_table_nth(((ZMapFeatureSet)feature_any)->features, 0))))
    {
      /* Cannot find feature from item. */

      zMapCritical("%s", "Could not launch blixem, cannot find any features in this column.") ;
    }
  else
    {
      int y1 ;
      ZMapFeature feature ;
      ZMapWindowCallbackCommandAlign align ;
      ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;


      /* We know this must be a feature by now. */
      feature = (ZMapFeature)feature_any ;

      align = g_new0(ZMapWindowCallbackCommandAlignStruct, 1) ;


      /* Set up general command field for callback. */
      align->cmd = ZMAPWINDOW_CMD_SHOWALIGN ;


      /* Set up all the parameters blixem needs to do it's display. */
      /* may be null if (temporary) blixem BAM option selected */
      if (feature)
	align->block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK) ;

      align->offset = window->sequence->start ;

      /* Work out where we are.... */
      if (feature && y_pos == 0.0)
		y_pos = (double)((feature->x1 + feature->x2) / 2) ;
	zmapWindowWorld2SeqCoords(window, (item ? item : focus_column), 0, y_pos, 0,0, NULL, &y1,NULL) ;
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
      else
	{
	  if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
	    {
	      align->homol_type = feature->feature.homol.type ;

	      align->homol_set = requested_homol_set ;
	    }
	  else
	    {
	      /* User may click on non-homol feature if they want to see some other feature + dna in blixem. */
	      align->homol_type = ZMAPHOMOL_N_HOMOL ;
	      
	      align->homol_set = ZMAPWINDOW_ALIGNCMD_NONE ;
	    }

	  align->feature_set = (ZMapFeatureSet)(feature->parent) ;

	  align->isSeq = zMapFeatureIsSeqFeatureSet(window->context_map,align->feature_set->unique_id);

	  /* If user clicked on features then make a list of them (may only be one), otherwise
	   * we need to use the feature set. */
	  if (selected_features == TRUE)
	    {
	      GList *focus_items ;

	      focus_items = zmapWindowFocusGetFocusItemsType(window->focus, WINDOW_FOCUS_GROUP_FOCUS) ;

	      align->features = zmapWindowItemListToFeatureListExpanded(focus_items,
									requested_homol_set == ZMAPWINDOW_ALIGNCMD_EXPANDED) ;

	      g_list_free(focus_items) ;
	    }
	}


      (*(window_cbs_G->command))(window, window->app_data, align) ;
    }


  return ;
}

