/*  File: zmapReadLine_P.h
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
 * Description: Internal data structs for the line reading package.
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_READLINE_p_h
#define ZMAP_READLINE_p_h


#include <glib.h>
#include <ZMap/zmapReadLine.hpp>


/* Represents a single C string from which to return successive lines. */
typedef struct _ZMapReadLineStruct
{
  char *original ;					    /* The original string. */
  char *current ;					    /* The current string which is
							       returned by zMapReadLineNext(). */
  char *next ;						    /* The string following the current
							       string. */

  gboolean in_place ;					    /* TRUE => return strings operating on
							       the original string and returning
							       pointers to substrings within it.*/
} ZMapReadLineStruct ;


#endif /* !ZMAP_READLINE_p_h */

