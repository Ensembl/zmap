/*  File: zmapControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * Description: Interface for creating, controlling and destroying ZMaps.
 *              
 * HISTORY:
 * Last edited: Jul  1 09:42 2004 (edgrif)
 * Created: Mon Nov 17 08:04:32 2003 (edgrif)
 * CVS info:   $Id: zmapControl.h,v 1.1 2004-07-01 08:42:53 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAPCONTROL_H
#define ZMAPCONTROL_H


/* It turns out we need some way to refer to zmapviews at this level, while I don't really
 * want to expose all the zmapview stuff I do want the opaque ZMapView type.
 * Think about this some more. */
#include <ZMap/zmapView.h>


/* Opaque type, represents an instance of a ZMap. */
typedef struct _ZMapStruct *ZMap ;


/* Applications can register functions that will be called back with their own
 * data and a reference to the zmap that made the callback. */
typedef void (*ZMapCallbackFunc)(ZMap zmap, void *app_data) ;



ZMap zMapCreate(void *app_data, ZMapCallbackFunc zmap_destroyed_cb) ;

/* NOW THIS IS WHERE WE NEED GERROR........ */
ZMapView zMapAddView(ZMap zmap, char *sequence) ;
gboolean zMapConnectView(ZMap zmap, ZMapView view) ;
gboolean zMapLoadView(ZMap zmap, ZMapView view) ;
gboolean zMapStopView(ZMap zmap, ZMapView view) ;
gboolean zMapDeleteView(ZMap zmap, ZMapView view) ;

char *zMapGetZMapID(ZMap zmap) ;
char *zMapGetZMapStatus(ZMap zmap) ;
gboolean zMapReset(ZMap zmap) ;
gboolean zMapDestroy(ZMap zmap) ;


#endif /* !ZMAPCONTROL_H */
