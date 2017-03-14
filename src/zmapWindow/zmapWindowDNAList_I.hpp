/*  File: zmapWindowDNAList_I.h
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

#ifndef __ZMAP_WINDOWDNALIST_I_H__
#define __ZMAP_WINDOWDNALIST_I_H__

#include <zmapWindowDNAList.hpp>
#include <zmapGUITreeView_I.hpp>      /* Finding this might be difficult? */

/*
 * Type checking and casting macros
 */
/* In public header now... */

/*
 * Main object structure
 */

typedef struct _zmapWindowDNAListStruct
{
  zmapGUITreeViewStruct __parent__;

  ZMapStyleMode feature_type;
} zmapWindowDNAListStruct;


/*
 * Class definition
 */

typedef struct _zmapWindowDNAListClassStruct {
  zmapGUITreeViewClassStruct __parent__;

} zmapWindowDNAListClassStruct;


#endif /* __ZMAP_WINDOWDNALIST_I_H__ */
