/*  File: zmapXMLAttributes.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 15 16:51 2006 (rds)
 * Created: Fri Aug  5 14:20:13 2005 (rds)
 * CVS info:   $Id: zmapXMLAttribute.c,v 1.3 2006-02-15 17:08:41 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>

zmapXMLAttribute zmapXMLAttribute_create(const XML_Char *name,
                                         const XML_Char *value)
{
  zmapXMLAttribute attr = NULL;

  attr = g_new0(zmapXMLAttributeStruct, 1);

  attr->name  = g_quark_from_string(g_ascii_strdown((char *)name, -1));
  attr->value = g_quark_from_string((char *)value);

  return attr;
}

void zmapXMLAttribute_free(zmapXMLAttribute attr)
{
  if(attr != NULL)
    {
      attr->name  = 0;
      attr->value = 0;
      g_free(attr);
    }
  return ;
}

GQuark zMapXMLAttribute_getValue(zmapXMLAttribute attr)
{
  return attr->value;
}

void zmapXMLAttributeMarkDirty(zmapXMLAttribute attr)
{
  attr->dirty = TRUE;
  return ;
}

/* Internal ! */
