/*  File: zmapWindowScratch_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Internal header for the window mark package..
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_MARK_P_H
#define ZMAP_WINDOW_MARK_P_H

#include <ZMap/zmapWindow.hpp>

/* Copy the given feature into the scratch column */
void zmapWindowScratchCopyFeature(ZMapWindow window, ZMapFeature new_feature, FooCanvasItem *item, const double x_pos_in, const double y_pos_in, const gboolean use_subfeature);
void zmapWindowScratchSetCDS(ZMapWindow window, ZMapFeature new_feature, FooCanvasItem *item, const double x_pos_in, const double y_pos_in, const gboolean use_subfeature, const gboolean set_cds_start, const gboolean set_cds_end);
void zmapWindowScratchDeleteFeature(ZMapWindow window, ZMapFeature new_feature, FooCanvasItem *item, const double x_pos_in, const double y_pos_in, const gboolean use_subfeature);
void zmapWindowScratchUndo(ZMapWindow window);
void zmapWindowScratchRedo(ZMapWindow window);
void zmapWindowScratchClear(ZMapWindow window);
ZMapFeatureSet zmapWindowScratchGetFeatureset(ZMapWindow window);
void zmapWindowScratchFeatureGetEvidence(ZMapWindow window, ZMapFeature feature, 
                                         ZMapWindowGetEvidenceCB evidence_cb, gpointer evidence_cb_dataw) ;

#endif	/* ZMAP_WINDOW_MARK_P_H */
