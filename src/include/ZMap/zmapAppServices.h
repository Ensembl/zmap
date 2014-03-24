/*  File: zmapAppServices.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012-2014: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Domain specific services called by main subsystems
 *              of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_SERVICES_H
#define ZMAP_APP_SERVICES_H

#include <gtk/gtk.h>

#include <ZMap/zmapFeatureLoadDisplay.h>




/* User callback function, called by zMapAppGetSequenceView code to return
 * a sequence, start, end and optionally config file specificed by user. */
typedef void (*ZMapAppGetSequenceViewCB)(ZMapFeatureSequenceMap sequence_map, gpointer user_data) ;


gboolean zMapAppGetSequenceConfig(ZMapFeatureSequenceMap seq_map, GError **error) ;
void zMapAppGetSequenceView(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map, gboolean display_sequence) ;
GtkWidget *zMapCreateSequenceViewWidg(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
				      ZMapFeatureSequenceMap sequence_map, gboolean display_sequence) ;
void zMapAppMergeSequenceName(ZMapFeatureSequenceMap seq_map_inout, const char *sequence_name, GError **error) ;
void zMapAppMergeSequenceCoords(ZMapFeatureSequenceMap seq_map, int start, int end, GError **error) ;

#endif /* !ZMAP_APP_SERVICES_H */
