/*  File: zmapXMLAttributes.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapXML_P.h>

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
