/*  File: zmapUtilsPrivate.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2017: Genome Research Ltd.
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
 * Description: Private header for utils, do not include directly.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_PRIVATE_H
#define ZMAP_UTILS_PRIVATE_H


/* Some compilers give more information than others so set up compiler dependant defines. */

#ifdef __GNUC__

#define ZMAP_MSG_CODE_FORMAT_STRING  "%s:%s:%d"

#define ZMAP_MSG_CODE_FORMAT_ARGS zmapGetFILEBasename(__FILE__), __PRETTY_FUNCTION__, __LINE__

#else /* __GNUC__ */

#define ZMAP_MSG_CODE_FORMAT_STRING  "%s:%d"

#define ZMAP_MSG_CODE_FORMAT_ARGS zmapGetFILEBasename(__FILE__), __LINE__

#endif /* __GNUC__ */


const char *zmapGetFILEBasename(const char *FILE_MACRO_ONLY) ;


#endif /* ZMAP_UTILS_PRIVATE_H */

