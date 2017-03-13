/*  File: zmapThreads.h
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

    // should we even have this ?
    bool ThreadStop() ;

    // Temp for old code....
    ZMapThread GetThread() ;

    bool HasFailed() ;

    ~ThreadSource() ;


    //-------------------------------------------------------------
    // Compatibility functions for old threads, will go....
    //
    ThreadSource(bool new_interface,
                 ZMapSlaveRequestHandlerFunc req_handler_func,
                 ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                 ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;

    bool ThreadStart() ;
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
