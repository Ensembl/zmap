/*  File: zmapAppconnect.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapAppServices.h>
#include <zmapApp_P.h>



static void createThreadCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data) ;



/* 
 *                 External interface.
 */

GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;

  
  frame = zMapCreateSequenceViewWidg(createThreadCB, app_context, sequence_map) ;


  return frame ;
}


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
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap seq_map)
{
  gboolean result = FALSE ;
  ZMap zmap ;
  ZMapManagerAddResult add_result ;

  /* Nothing specified on command line so check config file. */
  if (!(seq_map->sequence) && !(seq_map->start) && !(seq_map->end))
    zMapAppGetSequenceConfig(seq_map) ;


  /* Everything must be specified or nothing otherwise it's an error. */
  if (seq_map->sequence && seq_map->start && seq_map->end)
    {
      gboolean load_view = TRUE ;

      /* Hack...we need to make sure manager does _NOT_ load the view with the old xremote. */
      if (app_context->xremote_client)
	load_view = FALSE ;


      add_result = zMapManagerAdd(app_context->zmap_manager, seq_map, &zmap, load_view) ;
      if (add_result == ZMAPMANAGER_ADD_DISASTER)
	{
	  zMapWarning("%s", "Failed to create ZMap and then failed to clean up properly,"
		      " save your work and exit now !") ;
	}
      else if (add_result == ZMAPMANAGER_ADD_FAIL)
	{
	  zMapWarning("%s", "Failed to create ZMap") ;
	}
      else
	{
	  GtkTreeIter iter1 = {0} ;

	  gtk_tree_store_append(app_context->tree_store_widg, &iter1, NULL) ;
	  gtk_tree_store_set(app_context->tree_store_widg, &iter1,
			     ZMAPID_COLUMN, zMapGetZMapID(zmap),
			     ZMAPSEQUENCE_COLUMN,"<dummy>" ,
			     ZMAPSTATE_COLUMN, zMapGetZMapStatus(zmap),
			     ZMAPLASTREQUEST_COLUMN, "blah, blah, blaaaaaa",
			     ZMAPDATA_COLUMN, (gpointer)zmap,
			     -1) ;

#ifdef RDS_NEVER_INCLUDE_THIS_CODE
	  zMapDebug("GUI: create thread number %d for zmap \"%s\" for sequence \"%s\"\n",
		    (row + 1), row_text[0], row_text[1]) ;
#endif /* RDS_NEVER_INCLUDE_THIS_CODE */

	  result = TRUE ;
	}
    }
  else if (!(seq_map->sequence) && !(seq_map->start) && !(seq_map->end))
    {
      result = TRUE ;
    }
  else
    {
      result = FALSE ;

      zMapWarning("Sequence not specified properly: %s",
		  (!seq_map->sequence ? "no sequence name"
		   : (seq_map->start <= 1 ? "start less than 1" : "end less than start"))) ;
    }

  return result ;
}





/*
 *         Internal routines.
 */


static void createThreadCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  zmapAppCreateZMap(app_context, sequence_map) ;

  return ;
}



