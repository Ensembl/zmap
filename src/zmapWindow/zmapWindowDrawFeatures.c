/*  File: zmapWindowDrawFeatures.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Jul  1 10:27 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.1 2004-07-02 14:14:19 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>


static void zmapWindowProcessArray(GQuark key_id, gpointer data, gpointer user_data);



void zmapWindowDrawFeatures(ZMapWindow window, ZMapFeatureContext feature_context)
{
  float column_spacing = 10.0;
  GtkWidget *topWindow;
  char *title;

  if (!feature_context)
    return;

  title = g_strdup_printf("ZMap - %s", feature_context->sequence) ;
  topWindow = gtk_widget_get_toplevel(window->frame);
  gtk_window_set_title(GTK_WINDOW(topWindow), title) ;

  g_datalist_foreach(&(feature_context->features), zmapWindowProcessArray, &column_spacing);

  return;
}


static void zmapWindowProcessArray(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet zMapFeatureSet = (ZMapFeatureSet)data;
  float *column_spacing = (float*)user_data;
  int i;

  for (i = 0; i < zMapFeatureSet->features->len; i++)
    {
      //      printf("processing %s\n", zMapFeatureSet->features->name);
    }

  return;
}
/****************** end of file ************************************/
