/*  File: zmapWindowCommand.c
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

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <zmapWindow_P.hpp>


static gboolean doFilter(ZMapWindow window, ZMapWindowCallbackCommandFilter filter_data, char **err_msg_out) ;




/* 
 *                        External interface
 */


/* Called to execute a command on the given window, this is usually a request from View which is
 * often in response to a request from a ZMapWindow user action. Seems topsy-turvy but some
 * actions need to be executed on all windows within a view. */
gboolean zMapWindowExecuteCommand(ZMapWindow window, ZMapWindowCallbackCommandAny cmd_any, char **err_msg_out)
{
  gboolean result = FALSE ;
  ZMapWindowCallbackCommandFilter filter_data = (ZMapWindowCallbackCommandFilter)cmd_any ;

    switch (cmd_any->cmd)
    {
    case ZMAPWINDOW_CMD_COLFILTER:
      {
        if ((result = doFilter(window, filter_data, err_msg_out)))
          {
            /* Is there just one filtered column.....can only do it for one currently...
             * and filtering is being turned on then record the type of filtering...
             *  */
            window->filter_on = filter_data->do_filter ;

            if (filter_data->target_column && filter_data->do_filter)
              {
                window->filter_feature_set = filter_data->target_column ;
                window->filter_match_type = filter_data->match_type ;
                
                window->filter_selected = filter_data->selected ;
                window->filter_filter = filter_data->filter ;
                window->filter_action = filter_data->action ;
                window->filter_target = filter_data->target_type ;
              }
          }

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

/* Add or remove filtering to a column. */
static gboolean doFilter(ZMapWindow window, ZMapWindowCallbackCommandFilter filter_data, char **err_msg_out)
{
  gboolean result = FALSE ;

  filter_data->filter_result = zMapWindowContainerFeatureSetFilterFeatures(filter_data->match_type,
                                                                           filter_data->base_allowance,
                                                                           filter_data->selected,
                                                                           filter_data->filter,
                                                                           filter_data->action,
                                                                           filter_data->target_type,
                                                                           filter_data->filter_column,
                                                                           filter_data->filter_features,
                                                                           filter_data->target_column,
                                                                           filter_data->seq_start,
                                                                           filter_data->seq_end) ;
  


  /* Need reposition because we will have unfiltered the features first so they will need to
   * be redrawn even if we subsequently find no matches. */
  if (filter_data->filter_result == ZMAP_CANVAS_FILTER_OK
      || filter_data->filter_result == ZMAP_CANVAS_FILTER_NO_MATCHES)
    {
      zmapWindowFullReposition(window->feature_root_group, TRUE, "col filter") ;
    }

  if (filter_data->filter_result == ZMAP_CANVAS_FILTER_OK)
    result = filter_data->result = TRUE ;

  return result ;
}

