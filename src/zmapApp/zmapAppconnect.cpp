/*  File: zmapAppconnect.c
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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



static void createThreadCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data) ;



/*
 *                 External interface.
 */

GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;


  frame = zMapCreateSequenceViewWidg(createThreadCB, app_context, sequence_map, TRUE) ;


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


static void createThreadCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;

  zmapAppCreateZMap(app_context, sequence_map, &zmap, &view) ;

  return ;
}