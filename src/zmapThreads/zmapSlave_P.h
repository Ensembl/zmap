/*  File: zmapSlave_P.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Sep  5 17:31 2003 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapSlave_P.h,v 1.1 2003-11-13 15:02:13 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SLAVE_P_H
#define ZMAP_SLAVE_P_H

typedef struct
{
  ZMapConnection connection ;
  gboolean thread_died ;
  gchar *initial_error ;				    /* Holds string description of first
							       serious error that caused thread
							       termination. */

  ZMapServer server ;
  GString *server_request ;
  gchar *server_reply ;

} zmapThreadCBstruct, *zmapThreadCB ;




/* Error messages, needs some refining. */
#define ZMAPSLAVE_CONNCREATE "Connection creation failed"
#define ZMAPSLAVE_CONNOPEN   "Connection open failed"




#endif /* !ZMAP_SLAVE_P_H */
