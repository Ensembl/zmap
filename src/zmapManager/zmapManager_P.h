/*  File: zmapManager_P.h
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
 * Last edited: Nov  7 17:07 2003 (edgrif)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapManager_P.h,v 1.1 2003-11-10 17:04:28 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_MANAGER_P_H
#define ZMAP_MANAGER_P_H

#include <glib.h>
#include <ZMap/zmapConn.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapManager.h>


/* A list of connections of type ZMapConnection. */
typedef struct _ZMapManagerStruct
{
  GList *zmap_list ;

  zmapAppCallbackFunc delete_zmap_guifunc ;
  void *app_data ;

} ZMapManagerStruct ;



/* Each ZMap has window data and connection data, the intent is that the window code knows about
 * the window data and the connection data knows about the connection data.... */
typedef struct _ZMapWinConnStruct
{
  
  ZMapWindow window ;
  
  ZMapConnection connection ;
  
} ZMapWinConnStruct ;



typedef struct _ZMapManagerCBStruct
{
  ZMapManager zmap_list ;
  ZMapWinConn zmap ;
} ZMapManagerCBStruct, *ZMapManagerCB ;



void zmapCleanUpConnection(ZMapConnection connection, void *cleanupdata) ;
void zmapSignalData(ZMapConnection connection, void *cleanupdata) ;



#endif /* !ZMAP_MANAGER_P_H */
