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
 * Last edited: Sep 17 09:31 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapSlave_P.h,v 1.6 2004-09-17 08:32:19 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SLAVE_P_H
#define ZMAP_SLAVE_P_H

#include <glib.h>
#include <ZMap/zmapConn.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapServer.h>



typedef struct
{
  ZMapConnection connection ;
  gboolean thread_died ;
  gchar *initial_error ;				    /* Holds string description of first
							       serious error that caused thread
							       termination. */

  ZMapServer server ;					    /* The server for this thread..... */
} zmapThreadCBstruct, *zmapThreadCB ;




/* Error messages, needs some refining. */
#define ZMAPSLAVE_CONNCREATE  "Connection creation failed"
#define ZMAPSLAVE_CONNOPEN    "Connection open failed"
#define ZMAPSLAVE_CONNCONTEXT "Connection create context failed"
#define ZMAPSLAVE_CONNREQUEST "Connection request failed"




#endif /* !ZMAP_SLAVE_P_H */
