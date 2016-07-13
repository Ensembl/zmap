/*  File: zmapSlave_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SLAVE_P_H
#define ZMAP_SLAVE_P_H

#include <ZMap/zmapThreads.hpp>


typedef struct
{
  ZMapThread thread ;
  gboolean thread_died ;
  gchar *initial_error ;				    /* Holds string description of first
							       serious error that caused thread
							       termination. */

  /* User registered routine which the thread calls to handle requests and replies. */
  ZMapSlaveRequestHandlerFunc handler_func ;

  /* User registered routine to terminate thread if it needs to exit abnormally. */
  ZMapSlaveTerminateHandlerFunc terminate_func ;

  /* User registered routine to destroy/clean up thread if it needs to exit abnormally. */
  ZMapSlaveDestroyHandlerFunc destroy_func ;

  void *slave_data ;					    /* Any state required by slave. */

} zmapThreadCBstruct, *zmapThreadCB ;




/* Error messages, needs some refining and changing now thread bit is more generalised. */
#define ZMAPTHREAD_SLAVECREATE  "Thread creation failed"
#define ZMAPTHREAD_SLAVEOPEN    "Thread open failed"
#define ZMAPTHREAD_SLAVECONTEXT "Thread create context failed"
#define ZMAPTHREAD_SLAVEREQUEST "Thread request failed"


void *zmapNewThread(void *thread_args) ;



#endif /* !ZMAP_SLAVE_P_H */
