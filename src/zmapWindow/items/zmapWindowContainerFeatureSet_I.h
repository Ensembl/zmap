/*  File: zmapWindowItemFeatureSet_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Description: Internal header for class that represents a column
 *              in zmap.
 *
 * HISTORY:
 * Last edited: May 24 15:41 2010 (edgrif)
 * Created: Fri Feb  6 11:49:03 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerFeatureSet_I.h,v 1.13 2010-08-26 08:04:10 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__
#define __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__

#include <glib.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerFeatureSet.h>



typedef struct _zmapWindowContainerFeatureSetClassStruct
{
  zmapWindowContainerGroupClass __parent__;

} zmapWindowContainerFeatureSetClassStruct;



typedef struct _zmapWindowContainerFeatureSetStruct
{
  zmapWindowContainerGroup __parent__;


  ZMapWindow  window;
  ZMapStrand  strand ;
  ZMapFrame   frame ;
  GHashTable *style_table ;

  /* Empty columns are only hidden ATM and as they have no
   * ZMapFeatureSet removing them from the FToI hash becomes difficult
   * without the align, block and set ids. No doubt it'l be true for
   * empty block and align containers too at some point. */

  GQuark      align_id;
  GQuark      block_id;
  GQuark      unique_id;
  GQuark      original_id;

  /* We keep the features sorted by position and size so we can cursor through them... */
  gboolean    sorted;

  /* does the column contain featuresets that are maskable? */
  gboolean    maskable;
  /* does the column contain masked featuresets that have been masked (not displayed)? (default) */
  gboolean    masked;
  /* which featuresets are displayed in this column */
  GList *     featuresets;

  /* this is a column setting, the settings struct below is an amalgamation of styles */
  ZMapStyleBumpMode bump_mode;

  /* Extra items used for displaying colinearity lines and markers, note that we can end up
   * with the colinear markers becoming long items so we need to record them too. */
  GList *colinear_markers ;
  GList *incomplete_markers ;
  GList *splice_markers ;

  /* Features hidden by user, should stay hidden. */
  GQueue     *user_hidden_stack ;

  /* These fields are used for some of the more exotic column bumping. */
  gboolean    hidden_bump_features ; /* Features were hidden because they
				      * are out of the marked range. */

  struct
  {
    gboolean                    has_feature_set;
    gboolean                    has_stats;
    gboolean                    show_when_empty;
    gboolean                    frame_specific;
    gboolean                    strand_specific;
    gboolean                    show_reverse_strand;
    ZMapStyleColumnDisplayState bump_unmarked;
    ZMapStyle3FrameMode         frame_mode;
    ZMapStyleColumnDisplayState display_state;
    ZMapStyleBumpMode           bump_mode;
    ZMapStyleBumpMode           default_bump_mode;
  }settings;

} zmapWindowContainerFeatureSetStruct;



#endif /* __ZMAP_WINDOW_CONTAINER_FEATURE_SET_I_H__ */
