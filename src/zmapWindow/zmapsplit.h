/*  Last edited: Apr 15 11:50 2004 (rnc) */
/*  file: zmapsplit.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

#ifndef ZMAPSPLIT_H
#define ZMAPSPLIT_H

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <zmapcontrol.h>

/* function prototypes */

void  zoomIn          (GtkWindow *widget, gpointer data);
void  zoomOut         (GtkWindow *widget, gpointer data);
int   recordFocus     (GtkWidget *widget, GdkEvent *event, gpointer data); 
void  splitPane       (GtkWidget *widget, gpointer data);
void  splitHPane      (GtkWidget *widget, gpointer data);
void  closePane       (GtkWidget *widget, gpointer data);

#endif
/**************** end of file *************************/
