/*  File: zmapappconnect.c
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


/* Moving code to manager so need to somehow make sure that we signal app back so it
 * can update it's display...... */

/* Make a new zmap, if sequence is unspecified then the zmap will be blank and view
 * will not be set. */
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
			   ZMap *zmap_inout, ZMapView *view_out, char **err_msg_out)
{
  gboolean result = FALSE ;
  ZMap zmap = *zmap_inout ;
  ZMapView view = NULL ;
  GtkTreeIter iter1;
  ZMapManagerAddResult add_result ;

  add_result = zMapManagerAdd(app_context->zmap_manager, sequence_map, &zmap) ;
  if (add_result == ZMAPMANAGER_ADD_DISASTER)
    {
      zMapWarning("%s", "Failed to create ZMap and then failed to clean up properly,"
                  " save your work and exit now !") ;
    }
  else if (add_result == ZMAPMANAGER_ADD_FAIL)
    {
      *err_msg_out = g_strdup_printf("Bad start/end coords: %d, %d", sequence_map->start, sequence_map->end) ;
    }
  else
    {
      /* Set the text.  This is done even if it's already set, ah well! */
      if (sequence_map->sequence)
	gtk_entry_set_text(GTK_ENTRY(app_context->sequence_widg), sequence_map->sequence);


      add_result = zMapManagerAdd(app_context->zmap_manager, sequence_map, &zmap, &view) ;

      if (add_result == ZMAPMANAGER_ADD_DISASTER)
	{
	  *err_msg_out = g_strdup_printf("%s", "Failed to create ZMap and then failed to clean up properly,"
				     " save your work and exit now !") ;
	}
      else if (add_result == ZMAPMANAGER_ADD_FAIL)
	{
	  *err_msg_out = g_strdup_printf("%s", "Failed to create ZMap") ;
	}
      else
	{
	  /* agh....ok this not connected bit needs sorting out..... */


	  /* If we tried to load a sequence but couldn't connect then warn user, otherwise
	   * we just created the requested blank zmap. */
	  if (sequence_map->sequence && add_result == ZMAPMANAGER_ADD_NOTCONNECTED)
	    zMapWarning("%s", "ZMap added but could not connect to server, try \"Reload\".") ;

	  gtk_tree_store_append (app_context->tree_store_widg, &iter1, NULL);
	  gtk_tree_store_set (app_context->tree_store_widg, &iter1,
			      ZMAPID_COLUMN, zMapGetZMapID(zmap),
			      ZMAPSEQUENCE_COLUMN,"<dummy>" ,
			      ZMAPSTATE_COLUMN, zMapGetZMapStatus(zmap),
			      ZMAPLASTREQUEST_COLUMN, "blah, blah, blaaaaaa",
			      ZMAPDATA_COLUMN, (gpointer)zmap,
			      -1);

#ifdef RDS_NEVER_INCLUDE_THIS_CODE
	  zMapDebug("GUI: create thread number %d for zmap \"%s\" for sequence \"%s\"\n",
		    (row + 1), row_text[0], row_text[1]) ;
#endif /* RDS_NEVER_INCLUDE_THIS_CODE */

	  *zmap_inout = zmap ;

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

  zmapAppCreateZMap(app_context, sequence_map) ;

  return ;
}
