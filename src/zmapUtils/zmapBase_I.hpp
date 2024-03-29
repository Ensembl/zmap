/*  File: zmapBase_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#if MH17_NOT_NEEDED

#ifndef __ZMAP_BASE_I_H__
#define __ZMAP_BASE_I_H__

#include <ZMap/zmapBase.hpp>


typedef void (* ZMapBaseValueCopyFunc)(const GValue *src_value, GValue *dest_value);

typedef struct _zmapBaseStruct
{
  GObject __parent__;

  unsigned int debug : 1;

} zmapBaseStruct;

typedef struct _zmapBaseClassStruct
{
  GObjectClass __parent__;

  /* similar to gobject_class->set_property, but required for copy construction */
  GObjectSetPropertyFunc copy_set_property;

  /* Our version of the GTypeValueTable->value_copy function.
   * Ordinarily we'd just use the value_table member of GTypeInfo,
   * but threading scuppers that. */
  void (*value_copy)(const GValue *src_value, GValue *dest_value);

} zmapBaseClassStruct;

#endif /* __ZMAP_BASE_I_H__ */
#endif // MH17_NOT_NEEDED