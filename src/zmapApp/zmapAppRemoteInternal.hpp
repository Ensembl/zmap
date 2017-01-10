/*  File: zmapAppRemoteInternal.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_APP_INTERNAL_H
#define ZMAP_APP_INTERNAL_H


ZMapRemoteAppMakeRequestFunc zmapAppRemoteInternalGetRequestCB(void) ;
gboolean zmapAppRemoteInternalCreate(ZMapAppContext app_context, char *peer_socket, const char *peer_timeout_list_in) ;
gboolean zmapAppRemoteInternalInit(ZMapAppContext app_context) ;
void zmapAppRemoteInternalSetExitRoutine(ZMapAppContext app_context, ZMapAppRemoteExitFunc exit_routine) ;
void zmapAppRemoteInternalDestroy(ZMapAppContext app_context) ;





#endif /* !ZMAP_APP_INTERNAL_H */
