/*  File: zmapConn_P.h
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
 * Last edited: Nov 19 11:48 2003 (edgrif)
 * Created: Thu Jul 24 14:36:08 2003 (edgrif)
 * CVS info:   $Id: zmapConnNoThr_P.h,v 1.1 2005-02-02 11:26:27 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONN_PRIV_H
#define ZMAP_CONN_PRIV_H

#include <glib.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapSys.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapConn.h>


#define ZMAP_THR_DEBUG(ALL_ARGS_WITH_BRACKETS) \
do { \
     if (zmap_thr_debug_G) \
       printf ALL_ARGS_WITH_BRACKETS ; \
   } while (0)


typedef struct
{
  ZMapThreadRequest state ;				    /* Contains request to slave. */
} ZMapRequestStruct, *ZMapRequest ;

/* Replies via a simpler mutex. */
typedef struct
{
  ZMapThreadReply state ;				    /* Contains reply from slave. */
  char *data ;						    /* May also contain data for some replies. */
  char *error_msg ;					    /* May contain error message for when
							       thread fails. */
} ZMapReplyStruct, *ZMapReply ;



/* The ZMapConnection, there is one per connection to a database, i.e. one per slave
 * thread. */
typedef struct _ZMapConnectionStruct
{
  /* These are read only after creation of the thread. */
  gchar *machine ;
  int port ;
  gchar *sequence ;
  char *thread_id ;					    /* will contain connect addr. */


  /* These control the communication between the GUI thread and the connection threads,
   * they are mutex locked and the request code makes use of the condition variable. */
  ZMapRequestStruct request ;				    /* GUIs request. */
  ZMapReplyStruct reply ;				    /* Slaves reply. */

} ZMapConnectionStruct ;



#endif /* !ZMAP_CONN_PRIV_H */
