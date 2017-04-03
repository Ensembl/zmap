/*  File: zmapXMLAttribute.c
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

#include <ZMap/zmap.hpp>






#include <zmapXML_P.hpp>

ZMapXMLAttribute zmapXMLAttributeCreate(const XML_Char *name,
                                        const XML_Char *value)
{
  ZMapXMLAttribute attr = NULL;

  attr = g_new0(zmapXMLAttributeStruct, 1);

  attr->name  = g_quark_from_string(g_ascii_strdown((char *)name, -1));
  attr->value = g_quark_from_string((char *)value);

  return attr;
}

void zmapXMLAttributeFree(ZMapXMLAttribute attr)
{
  if(attr != NULL)
    {
      attr->name  = 0;
      attr->value = 0;
      g_free(attr);
    }
  return ;
}

GQuark zMapXMLAttributeGetValue(ZMapXMLAttribute attr)
{
  return attr->value;
}

void zmapXMLAttributeMarkDirty(ZMapXMLAttribute attr)
{
  attr->dirty = TRUE;
  return ;
}

/* Internal ! */
