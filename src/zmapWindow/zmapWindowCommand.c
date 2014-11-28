/*  File: zmapWindowCommand.c
 *  Author: ed (edgrif@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *              
 * Description: Implements a simple command processor for commands
 *              coming from the layer above this (zmapView).
 *
 * Exported functions: See zmapWindow.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <zmapWindow_P.h>





static gboolean doSplice(ZMapWindow window, ZMapWindowCallbackCommandSplice splice_data, char **err_msg_out) ;



/* 
 *                        External interface
 */

/* Called to execute a command on the given window, this is usually a request from View which is
 * often in response to a request from a ZMapWindow user action. Seems topsy-turvy but some
 * actions need to be executed on all windows within a view. */
gboolean zMapWindowExecuteCommand(ZMapWindow window, ZMapWindowCallbackCommandAny cmd_any, char **err_msg_out)
{
  gboolean result = FALSE ;

  switch (cmd_any->cmd)
    {
    case ZMAPWINDOW_CMD_SPLICE:
      {
        ZMapWindowCallbackCommandSplice splice_data = (ZMapWindowCallbackCommandSplice)cmd_any ;

        result = doSplice(window, splice_data, err_msg_out) ;

        break ;
      }

    default:
      {
        zMapWarnIfReached() ;

        result = FALSE ;
      }
    }

  return result ;
}





/* 
 *                    Internal routines.
 */


/* Add or remove splice highlighting to a window. */
static gboolean doSplice(ZMapWindow window, ZMapWindowCallbackCommandSplice splice_data, char **err_msg_out)
{
  gboolean result = FALSE ;
  FooCanvasGroup *focus_column ;
  ZMapWindowContainerFeatureSet focus_container ;

  if (splice_data->do_highlight)
    {
      FooCanvasItem *focus_item  ;

      if ((focus_column = zmapWindowFocusGetHotColumn(window->focus))
          && (focus_item = zmapWindowFocusGetHotItem(window->focus)))
        {
          ZMapWindowContainerFeatureSet focus_container ;
          gboolean result ;

          /* Record the current hot item. */
          window->focus_column = focus_column ;
          window->focus_item = focus_item ;
          window->focus_feature = zMapWindowCanvasItemGetFeature(focus_item) ;

          focus_container = (ZMapWindowContainerFeatureSet)focus_column ;

          if ((result = zmapWindowContainerFeatureSetSpliceHighlightFeatures(focus_container,
                                                                             splice_data->highlight_features,
                                                                             splice_data->seq_start,
                                                                             splice_data->seq_end)))
            {
              zmapWindowFullReposition(window->feature_root_group, TRUE, "key s") ;

              window->splice_highlight_on = TRUE ;
            }
          else
            {
              *err_msg_out = g_strdup_printf("%s", "No columns configured to show splice highlights.") ;
            }
        }
    }
  else
    {
      if ((focus_column = zmapWindowFocusGetHotColumn(window->focus))
          || (focus_column = zmapWindowGetFirstColumn(window, ZMAPSTRAND_FORWARD)))
        {
          focus_container = (ZMapWindowContainerFeatureSet)focus_column ;

          if ((result = zmapWindowContainerFeatureSetSpliceUnhighlightFeatures(focus_container)))
            {
              zmapWindowFullReposition(window->feature_root_group, TRUE, "key S") ;

              window->splice_highlight_on = FALSE ;
            }
          else
            {
              *err_msg_out = g_strdup_printf("%s", "No columns configured to show splice highlights.") ;
            }
        }
    }

  return result ;
}
