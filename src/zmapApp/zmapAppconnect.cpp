/*  File: zmapAppconnect.c
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
 * Description: Creates sequence chooser panel of zmap application
 *              window.
 * Exported functions: See zmapApp_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <zmapApp_P.hpp>



static void createThreadCB(ZMapFeatureSequenceMap sequence_map, 
                           const bool recent_only,
                           gpointer user_data) ;



/*
 *                 External interface.
 */

GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;


  frame = zMapCreateSequenceViewWidg(createThreadCB, app_context,
                                     NULL, NULL, sequence_map) ;


  return frame ;
}


/* Make a new zmap, if sequence is unspecified then the zmap will be blank and view
 * will not be set. */
/* sequence etc can be unspecified to create a blank zmap. */
/* Sequence must be fully specified in seq_map as sequence/start/end
 *
 * and config_file....investigate more thoroughly....probably needs setting....
 * check in debugger.....
 *
 * Returns TRUE if sequence correctly specified or no sequence at all specified,
 * only returns FALSE if sequence incorrectly specified.
 *
 *
 *  */
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
                           ZMap *zmap_out, ZMapView *view_out)
{
  gboolean result = FALSE ;
  ZMap zmap = *zmap_out ;
  ZMapView view = NULL ;
  ZMapManagerAddResult add_result ;
  GError *tmp_error = NULL ;

  /* Nothing specified on command line so check config file. */
  if (!(sequence_map->sequence) && !(sequence_map->start) && !(sequence_map->end))
    zMapAppGetSequenceConfig(sequence_map, &tmp_error) ;

  if (tmp_error)
    {
      result = FALSE ;
      zMapWarning("%s", tmp_error->message) ;
    }
  else if (!((sequence_map->sequence && sequence_map->start && sequence_map->end)
        || (!(sequence_map->sequence) && !(sequence_map->start) && !(sequence_map->end))))
    {
      /* Everything must be specified or nothing otherwise it's an error. */
      result = FALSE ;

      zMapWarning("Sequence not specified properly: %s",
          (!sequence_map->sequence ? "no sequence name"
           : (sequence_map->start <= 1 ? "start less than 1" : "end less than start"))) ;
    }
  else if ((!(sequence_map->sequence) && !(sequence_map->start) && !(sequence_map->end)))
    {
      result = TRUE ;
    }
  else
    {
      add_result = zMapManagerAdd(app_context->zmap_manager, sequence_map, &zmap, &view, &tmp_error) ;

      if (add_result == ZMAPMANAGER_ADD_DISASTER)
        {
          zMapWarning("%s", "Failed to create ZMap and then failed to clean up properly,"
              " save your work and exit now !") ;
        }
      else if (add_result == ZMAPMANAGER_ADD_FAIL)
        {
          zMapWarning("Failed to create ZMap for sequence '%s' [%d, %d]: %s", 
                      sequence_map->sequence, sequence_map->start, sequence_map->end, 
                      (tmp_error ? tmp_error->message : "<no error message>")) ;
        }
      else
        {
          
          zmapAppManagerUpdate(app_context, zmap, view) ;


          /* Need to update manager..... */
          *zmap_out = zmap ;
          if (view)
            *view_out = view ;
        
          result = TRUE ;
        }
    }

  return result ;
}




/*
 *         Internal routines.
 */


static void createThreadCB(ZMapFeatureSequenceMap sequence_map, 
                           const bool recent_only,
                           gpointer user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;

  zmapAppCreateZMap(app_context, sequence_map, &zmap, &view) ;

  return ;
}
