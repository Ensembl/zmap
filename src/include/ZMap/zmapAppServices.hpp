/*  File: zmapAppServices.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Domain specific services called by main subsystems
 *              of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_SERVICES_H
#define ZMAP_APP_SERVICES_H

#include <gtk/gtk.h>

#include <ZMap/zmapFeatureLoadDisplay.hpp>


/* Types of source available to add on the Create Source dialog */
enum class ZMapAppSourceType
{
  NONE,
  FILE, 
#ifdef USE_ENSEMBL
  ENSEMBL, 
#endif
  TRACKHUB
} ;



/* User callback function, called by zMapAppGetSequenceView code to return
 * a sequence, start, end and optionally config file specificed by user. */
typedef void (*ZMapAppGetSequenceViewCB)(ZMapFeatureSequenceMap sequence_map, const bool recent_only, gpointer user_data) ;
typedef void (*ZMapAppClosedSequenceViewCB)(GtkWidget *toplevel, gpointer user_data) ;

/* User callback function, called by zMapAppCreateSource code */
typedef void (*ZMapAppCreateSourceCB)(const char *name, const std::string &url, 
                                      const char *featuresets, const char *biotypes,
                                      const std::string &file_type, const int num_fields,
                                      gpointer user_data, GError **error) ;


GtkWidget *zMapAppGetSequenceView(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                                  ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                                  ZMapFeatureSequenceMap sequence_map, gboolean display_sequence) ;
GtkWidget *zMapCreateSequenceViewWidg(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                                      ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                                      ZMapFeatureSequenceMap sequence_map, 
                                      gboolean display_sequence = TRUE, const gboolean import = FALSE,
                                      GtkWidget *toplevel = NULL) ;

gboolean zMapAppGetSequenceConfig(ZMapFeatureSequenceMap seq_map, GError **error) ;

GtkWidget *zMapAppCreateSource(ZMapFeatureSequenceMap sequence_map, 
                               ZMapAppCreateSourceCB user_func, gpointer user_data,
                               ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
#ifdef USE_ENSEMBL
                               ZMapAppSourceType default_type = ZMapAppSourceType::ENSEMBL
#else
                               ZMapAppSourceType default_type = ZMapAppSourceType::TRACKHUB
#endif
                               ) ;
GtkWidget *zMapAppEditSource(ZMapFeatureSequenceMap sequence_map, ZMapConfigSource source,
                             ZMapAppCreateSourceCB user_func, gpointer user_data) ;

void zMapAppMergeSequenceName(ZMapFeatureSequenceMap seq_map_inout, const char *sequence_name, 
                              const gboolean merge_details, GError **error) ;
void zMapAppMergeSequenceCoords(ZMapFeatureSequenceMap seq_map, int start, int end, 
                                const gboolean exact_match, const gboolean merge_details, GError **error) ;

#endif /* !ZMAP_APP_SERVICES_H */
