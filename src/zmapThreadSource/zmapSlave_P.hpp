/*  File: zmapSlave_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: 
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SLAVE_P_H
#define ZMAP_SLAVE_P_H

#include <glib.h>

#include <ZMap/zmapThreadSource.hpp>



typedef struct
{
  ZMapThread thread ;

  // Used to detect whether thread was cancelled or exitted under control.
  bool thread_cancelled ;

  // true if the underlying data source has died.
  gboolean server_died ;

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



extern bool zmap_threadsource_debug_G ;


void *zmapNewThread(void *thread_args) ;



#endif /* !ZMAP_SLAVE_P_H */
