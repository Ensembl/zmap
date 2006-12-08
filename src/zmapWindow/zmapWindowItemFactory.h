/*  File: zmapWindowItemFactory.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Dec  7 10:12 2006 (rds)
 * Created: Mon Sep 25 09:09:52 2006 (rds)
 * CVS info:   $Id: zmapWindowItemFactory.h,v 1.4 2006-12-08 15:39:59 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWITEM_FACTORY_H
#define ZMAPWINDOWITEM_FACTORY_H

#define FACTORY_METHOD_COUNT ZMAPFEATURE_PEP_SEQUENCE


typedef void (*ZMapWindowFToIFactoryItemCreated)(FooCanvasItem            *new_item,
                                                 ZMapWindowItemFeatureType new_item_type,
                                                 ZMapFeature               full_feature,
                                                 ZMapWindowItemFeature     sub_feature,
                                                 double                    new_item_y1,
                                                 double                    new_item_y2,
                                                 gpointer                  handler_data);

typedef gboolean (*ZMapWindowFToIFactoryTopItemCreated)(FooCanvasItem *top_item,
                                          ZMapFeatureContext context,
                                          ZMapFeatureAlignment align,
                                          ZMapFeatureBlock block,
                                          ZMapFeatureSet set,
                                          ZMapFeature feature,
                                          gpointer handler_data);

typedef gboolean (*ZMapWindowFToIFactoryItemFeatureSizeRequest)(ZMapFeature feature, 
                                                  double *limits_array, 
                                                  double *points_array_inout, 
                                                  gpointer handler_data);

typedef struct
{
  ZMapWindowFToIFactoryItemCreated item_created;
  ZMapWindowFToIFactoryTopItemCreated top_item_created;
  ZMapWindowFToIFactoryItemFeatureSizeRequest feature_size_request;
}ZMapWindowFToIFactoryProductionTeamStruct, *ZMapWindowFToIFactoryProductionTeam;


ZMapWindowFToIFactory zmapWindowFToIFactoryOpen(GHashTable *feature_to_item_hash,
                                                ZMapWindowLongItems long_items);
void zmapWindowFToIFactorySetup(ZMapWindowFToIFactory factory, 
                                guint line_width, /* replace with a config struct ? */
                                ZMapWindowStats stats,
                                ZMapWindowFToIFactoryProductionTeam signal_handlers, 
                                gpointer handler_data);
void zmapWindowFToIFactoryRunSet(ZMapWindowFToIFactory factory, 
                                 ZMapFeatureSet set, 
                                 FooCanvasGroup *container,
                                 ZMapFrame frame);
FooCanvasItem *zmapWindowFToIFactoryRunSingle(ZMapWindowFToIFactory factory, 
                                              FooCanvasGroup       *parent_container,
                                              ZMapFeatureContext    context, 
                                              ZMapFeatureAlignment  align, 
                                              ZMapFeatureBlock      block,
                                              ZMapFeatureSet        set,
                                              ZMapFeature           feature);
void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory);


#endif /* #ifndef ZMAPWINDOWITEM_FACTORY_H */
