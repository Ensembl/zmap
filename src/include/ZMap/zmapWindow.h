/*  File: zmapWindow.h
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
 * and was written by
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jan 23 13:21 2004 (edgrif)
 * Created: Thu Jul 24 15:21:56 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.h,v 1.1 2004-03-03 12:51:52 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <ZMap/zmapSys.h>				    /* For callback funcs... */

/* Window stuff, callbacks.... */
typedef enum {ZMAP_WINDOW_INIT, ZMAP_WINDOW_LOAD,
	      ZMAP_WINDOW_STOP, ZMAP_WINDOW_QUIT} ZmapWindowCmd ;


/* Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;


ZMapWindow zMapWindowCreate(char *sequence, char *zmap_id,
			    zmapVoidIntCallbackFunc app_routine, void *app_data) ;
void zMapWindowSignalData(ZMapWindow window, void *data) ;
void zMapWindowDestroy(ZMapWindow window) ;

#endif /* !ZMAP_WINDOW_H */
