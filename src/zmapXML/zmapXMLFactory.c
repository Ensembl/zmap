/*  File: zmapXMLFactory.c
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
 * Last edited: Aug 26 16:08 2005 (rds)
 * Created: Tue Aug 16 14:40:59 2005 (rds)
 * CVS info:   $Id: zmapXMLFactory.c,v 1.1 2005-09-05 17:28:22 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>


/* This might mean we can make elements private */
int zMapXMLFactoryDecodeElement(GHashTable *userTypesTable, 
                                zmapXMLElement element,
                                GList **listout)
{
  return zMapXMLFactoryListFromNameQuark(userTypesTable, element->name, listout);

  zmapXMLFactoryItem ptype = NULL;
  int type = -1;

  ptype = g_hash_table_lookup(userTypesTable, 
                              GINT_TO_POINTER(element->name));
  if(ptype)
    {
      type = ptype->type;
      if(listout)
        *listout = ptype->list;
    }

  return type;
}

int zMapXMLFactoryListFromNameQuark(GHashTable *userTypesTable, 
                                    GQuark name,
                                    GList **listout)
{
  zmapXMLFactoryItem ptype = NULL;
  int type = -1;

  ptype = g_hash_table_lookup(userTypesTable, 
                              GINT_TO_POINTER(name));
  if(ptype)
    {
      type = ptype->type;
      if(listout)
        *listout = ptype->list;
    }

  return type;
}



void zMapXMLFactory_listAppend(GHashTable *userTypesTable, 
                               zmapXMLElement element, 
                               void *listItem)
{
  zmapXMLFactoryItem ptype = NULL;

  if(!listItem)
    return ;

  ptype = g_hash_table_lookup(userTypesTable, 
                              GINT_TO_POINTER(element->name));
  if(ptype)
    ptype->list = g_list_append(ptype->list, listItem);

  return ;
}
