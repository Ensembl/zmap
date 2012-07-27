/*  File: zmapWindowItemFactory.c
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWITEM_FACTORY_H
#define ZMAPWINDOWITEM_FACTORY_H


typedef gboolean (*ZMapWindowFToIFactoryTopItemCreated)(FooCanvasItem *top_item,
                                          ZMapWindowFeatureStack feature_stack,
                                          gpointer handler_data);

#if FEATURE_SIZE_REQUEST
typedef gboolean (*ZMapWindowFToIFactoryItemFeatureSizeRequest)(ZMapFeature feature,
                                                  double *limits_array,
                                                  double *points_array_inout,
                                                  gpointer handler_data);
#endif
typedef struct
{
  ZMapWindowFToIFactoryTopItemCreated top_item_created;
#if FEATURE_SIZE_REQUEST
  ZMapWindowFToIFactoryItemFeatureSizeRequest feature_size_request;
#endif
}ZMapWindowFToIFactoryProductionTeamStruct, *ZMapWindowFToIFactoryProductionTeam;


ZMapWindowFToIFactory zmapWindowFToIFactoryOpen(GHashTable *feature_to_item_hash);

void zmapWindowFToIFactorySetup(ZMapWindowFToIFactory factory,
                                guint line_width, /* replace with a config struct ? */
                                ZMapWindowFToIFactoryProductionTeam signal_handlers,
                                gpointer handler_data);
#if RUN_SET
void zmapWindowFToIFactoryRunSet(ZMapWindowFToIFactory factory,
                                 ZMapFeatureSet set,
                                 FooCanvasGroup *container,
                                 ZMapFrame frame);
#endif
FooCanvasItem *zmapWindowFToIFactoryRunSingle(GHashTable *ftoi_hash,
#if RUN_SET
							FooCanvasItem        *current_item,
#endif
                                              FooCanvasGroup       *parent_container,
                                              ZMapWindowFeatureStack feature_stack);

void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory);


#endif /* #ifndef ZMAPWINDOWITEM_FACTORY_H */
