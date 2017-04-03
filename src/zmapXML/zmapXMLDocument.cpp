/*  File: zmapXMLDocument.c
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

ZMapXMLDocument zMapXMLDocumentCreate(const XML_Char *version,
                                      const XML_Char *encoding,
                                      int standalone)
{
  ZMapXMLDocument doc = NULL;
  doc = g_new0(zmapXMLDocumentStruct, 1);

  doc->standalone = standalone;
  if(version)
    doc->version  = g_quark_from_string(version);
  if(encoding)
    doc->encoding = g_quark_from_string(encoding);

  return doc;
}

void zMapXMLDocumentSetRoot(ZMapXMLDocument doc,
                            ZMapXMLElement root)
{
  if(!(doc->root))
    doc->root = root;
  else
    printf("can't set root more than once!\n");
  return ;
}

char *zMapXMLDocumentVersion(ZMapXMLDocument doc)
{
  return g_strdup(g_quark_to_string(doc->version));
}

char *zMapXMLDocumentEncoding(ZMapXMLDocument doc)
{
  return g_strdup(g_quark_to_string(doc->encoding));
}

gboolean zMapXMLDocumentIsStandalone(ZMapXMLDocument doc)
{
  if(doc->standalone)
    return TRUE;
  else
    return FALSE;
}


void zMapXMLDocumentReset(ZMapXMLDocument doc)
{
  printf("This code isn't written (reset)\n");
  return ;
}
void zMapXMLDocumentDestroy(ZMapXMLDocument doc)
{
  printf("This code isn't written (destroy)\n");
  return ;
}
