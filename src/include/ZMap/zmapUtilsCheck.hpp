/*  File: zmapUtilsCheck.h
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
 * Description: Contains macros, functions etc. for checking in production
 *              code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_CHECK_H
#define ZMAP_UTILS_CHECK_H

#include <stdlib.h>
#include <glib.h>

#include <ZMap/zmapUtilsPrivate.hpp>



/* Use to test functions, e.g.
 *         zmapCheck(myfunc("x y z"), "%s", "disaster, my func should never fail") ;
 *  */
#define zMapCheck(EXPR, FORMAT, ...)                                 \
  G_STMT_START{                                                      \
  if ((EXPR))                                                        \
    {                                                                \
      g_printerr(ZMAP_MSG_CODE_FORMAT_STRING " '%s' " FORMAT,        \
                 ZMAP_MSG_CODE_FORMAT_ARGS,                          \
                 #EXPR,                                              \
                 __VA_ARGS__);                                       \
      exit(EXIT_FAILURE) ;                                           \
    };                                                               \
}G_STMT_END


#endif /* ZMAP_UTILS_CHECK_H */
