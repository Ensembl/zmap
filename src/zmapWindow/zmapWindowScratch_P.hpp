/*  File: zmapWindowScratch_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
