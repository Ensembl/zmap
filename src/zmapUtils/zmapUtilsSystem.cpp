/*  File: zmapUtilsSystem.c
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
 * Description: Contains routines for various system functions.
 *
 * Exported functions: See zmapUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <sys/utsname.h>

#include <zmapUtils_P.hpp>




/*
 *               External Interface functions
 */



/* Check the system type, e.g. Linux, Darwin etc, the string returned is as per
 * the uname command so you should test it against the string you expect.
 * The function returns NULL if the uname command failed, otherwise it returns
 * a string that should be g_free'd by the caller. */
char *zMapUtilsSysGetSysName(void)
{
  char *sys_name = NULL ;
  struct utsname unamebuf ;

  if (uname(&unamebuf) == 0)
    {
      sys_name = g_strdup(unamebuf.sysname) ;
    }

  return sys_name ;
}






/*
 *               Internal functions
 */

