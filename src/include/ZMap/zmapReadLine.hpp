/*  File: zmapReadLine.h
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
 * Description: The interface to a line reading package.
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_READLINE_H
#define ZMAP_READLINE_H


#include <glib.h>


/*! @addtogroup zmaputils
 * @{
 *  */


/*!
 * ZMapReadLine object containing the set of lines to be returned one at a time. */
typedef struct _ZMapReadLineStruct *ZMapReadLine ;


/*! @} end of zmaputils docs. */


ZMapReadLine zMapReadLineCreate(char *lines, gboolean in_place) ;
gboolean zMapReadLineNext(ZMapReadLine read_line, char **next_line_out, gsize *line_length_out) ;
void zMapReadLineDestroy(ZMapReadLine read_line, gboolean free_string) ;

#endif /* !ZMAP_READLINE_H */
