/*  File: zmapThreads.h
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
 * Description: High level interface to running threads to fetch data.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_H
#define ZMAP_THREAD_H

#include <config.h>
#include <glib.h>

#include <ZMap/zmapEnum.hpp>
#include <ZMap/zmapThreadsLib.hpp>







namespace ZMapThreadSource
{

// User function to call when slave has replied.
typedef void (*ZMapThreadPollSlaveUserReplyFunc)(void *func_data) ;


  class ThreadSource
  {

  public:

    // I should get rid of terminate and destroy....but that's a bigger change.
    ThreadSource(ZMapSlaveRequestHandlerFunc req_handler_func,
                 ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                 ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;

    // why public ??
    static bool sourceCheck(ThreadSource &thread_source) ;

    bool ThreadStart(ZMapThreadPollSlaveUserReplyFunc user_reply_func,
                     void *user_reply_func_data) ;

    bool SendRequest(void *request) ;

    bool HasFailed() ;

    bool ThreadStop() ;

    ~ThreadSource() ;


    //-------------------------------------------------------------
    // Compatibility functions for old threads, will go....
    //
    ThreadSource(bool new_interface,
                 ZMapSlaveRequestHandlerFunc req_handler_func,
                 ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                 ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;

    bool ThreadStart() ;


    // Temp for old code....
    ZMapThread GetThread() ;
    //-------------------------------------------------------------


  protected:


  private:

    enum class ThreadSourceStateType {INVALID, INIT, POLLING, STOPPING, STOPPED} ;
    
    typedef unsigned int ZMapPollSlaveID ;


    // No default constructor.
    ThreadSource() = delete ;

    // No copy operations
    ThreadSource(const ThreadSource&) = delete ;
    ThreadSource& operator=(const ThreadSource&) = delete ;

    // no move operations
    ThreadSource(ThreadSource&&) = delete ;
    ThreadSource& operator=(ThreadSource&&) = delete ;

    void StopPolling() ;


    // Data

    bool failed_ ;

    ThreadSourceStateType state_ ;

    ZMapThread thread_ ;

    ZMapPollSlaveID poll_id_ ;

    ZMapThreadPollSlaveUserReplyFunc user_reply_func_ ;

    void *user_reply_func_data_ ;

  } ;


} /* ZMapThreadSource namespace */

#endif /* !ZMAP_THREAD_H */
