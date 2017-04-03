/*  File: zmapUtilsLogical.h
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
 * Description: Macros for manipulating bits and other logic things.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_LOGICAL_H
#define ZMAP_UTILS_LOGICAL_H


/* Turn bits on/off. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_FLAG_ON(VAR, FLAG) \
  (VAR) |= (FLAG)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
#define ZMAP_FLAG_ON(TYPE, VAR, FLAG) \
(VAR) = (TYPE)((VAR) | (FLAG))


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_FLAG_OFF(VAR, FLAG) \
  (VAR) &= ~(FLAG)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
#define ZMAP_FLAG_OFF(TYPE, VAR, FLAG)          \
  (VAR) = (TYPE)((VAR) & ~(FLAG))



/* Test bits */
#define ZMAP_FLAG_IS_ON(VAR, FLAG) \
  ((VAR) & (FLAG))


#endif /* ZMAP_UTILS_LOGICAL_H */
