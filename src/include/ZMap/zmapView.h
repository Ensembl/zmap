/*  File: zmapView.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Interface for controlling a single "view", a view
 *              comprises one or more windowsw which display data
 *              collected from one or more servers. Hence the view
 *              interface controls both windows and connections to
 *              servers.
 *              
 * HISTORY:
 * Last edited: May 19 12:00 2004 (edgrif)
 * Created: Thu May 13 14:59:14 2004 (edgrif)
 * CVS info:   $Id: zmapView.h,v 1.1 2004-05-19 11:59:48 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAPVIEW_H
#define ZMAPVIEW_H


/* Opaque type, represents an instance of a ZMapView. */
typedef struct _ZMapViewStruct *ZMapView ;


typedef void (*ZMapViewCallbackFunc)(ZMapView zmap_view, void *app_data) ;


ZMapView zMapViewCreate(GtkWidget *parent_widget, char *sequence,
			void *app_data, ZMapViewCallbackFunc destroy_cb) ;

gboolean zMapViewConnect(ZMapView zmap_view) ;

gboolean zMapViewLoad(ZMapView zmap_view, char *sequence) ; /* sequence == NULL => reload existing
							       sequence. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Don't exist but will be needed. */

gboolean zMapViewAddViewWindow(ZMapView zmap_view) ;
gboolean zMapViewDeleteViewWindow(ZMapView zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


gboolean zMapViewReset(ZMapView zmap_view) ;

char *zMapViewGetSequence(ZMapView zmap_view) ;
char *zMapViewGetStatus(ZMapView zmap_view) ;

gboolean zMapViewDestroy(ZMapView zmap_view) ;


#endif /* !ZMAPVIEW_H */
