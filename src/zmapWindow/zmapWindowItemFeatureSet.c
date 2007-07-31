/*  File: zmapWindowItemFeatureSet.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jul 30 16:50 2007 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.c,v 1.1 2007-07-31 16:21:15 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>

static void removeList(gpointer data, gpointer user_data_unused) ;


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
                                                            ZMapFeatureTypeStyle style,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame)
{
  ZMapWindowItemFeatureSetData item_feature_set = NULL;

  if((item_feature_set = g_new0(ZMapWindowItemFeatureSetDataStruct, 1)))
    {
      item_feature_set->strand      = strand;
      item_feature_set->frame       = frame;
      item_feature_set->style       = zMapFeatureStyleCopy(style) ;
      item_feature_set->style_table = zmapWindowStyleTableCreate() ;
      item_feature_set->window      = window;
      item_feature_set->user_hidden_stack = g_queue_new();
    }

  return item_feature_set;
}

void zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set)
{
  item_feature_set->window = NULL; /* not ours ... */

  if(item_feature_set->style)
    zMapFeatureTypeDestroy(item_feature_set->style);

  if(item_feature_set->style_table)
    zmapWindowStyleTableDestroy(item_feature_set->style_table);

  /* Get rid of stack or user hidden items. */
  if(!g_queue_is_empty(item_feature_set->user_hidden_stack))
    g_queue_foreach(item_feature_set->user_hidden_stack, removeList, NULL) ;

  if(item_feature_set->user_hidden_stack)
    g_queue_free(item_feature_set->user_hidden_stack);

  g_free(item_feature_set);

  return ;
}


/* INTERNAL */

static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}


