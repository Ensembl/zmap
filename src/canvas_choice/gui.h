/*  File: gui.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include <utils.h>

#define DEMO_PACK_SIZE (5)

typedef struct _demoGUIStruct
{
  GtkWidget *toplevel, *vbox ;
  GtkWidget *zoom_in, *zoom_out, *close;

  gpointer       user_data;
  GDestroyNotify user_destroy_notify;
}demoGUIStruct, *demoGUI;

typedef GtkWidget *(*demoGUICallbackFunc)(GtkWidget *canvas_container, gpointer user_data);

typedef struct _demoGUICallbackSetStruct
{
  demoGUICallbackFunc create_canvas;
  GCallback draw_random_items;
  GCallback zoom_in;
  GCallback zoom_out;
  GCallback spare_1;
  GCallback spare_2;
  GDestroyNotify data_destroy_notify;
}demoGUICallbackSetStruct, *demoGUICallbackSet;

demoGUI demoGUICreate(char *title, demoGUICallbackSet callbacks, gpointer user_data);

#endif /* GUI_H */
