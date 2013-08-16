/*  File: zmapXMLDocument.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapXML_P.h>

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
