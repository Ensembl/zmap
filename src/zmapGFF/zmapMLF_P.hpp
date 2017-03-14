/*  File: zmapMLF_P.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * Description: Header file for multi-line feature data structure. This
 * is the private header, so simply contains the details of the data
 * structure itself. The public header with the function interface is
 * the file '/src/include/ZMap/zmapMLF.h'.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_MLF_P_H
#define ZMAP_MLF_P_H

#include <string.h>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapMLF.hpp>



typedef struct ZMapMLFStruct_
  {
    GHashTable *pIDTable ;
  } ZMapMLFStruct ;


#endif
