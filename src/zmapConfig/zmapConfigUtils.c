/*  File: zmapConfigUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 31 15:34 2007 (edgrif)
 * Created: Tue Apr  6 12:30:05 2004 (edgrif)
 * CVS info:   $Id: zmapConfigUtils.c,v 1.4 2007-05-31 14:36:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>


static void glistFreeElement(gpointer data, gpointer user_data) ;


/* Make an empty stanza. */
ZMapConfigStanza zmapConfigCreateStanza(char *stanza_name)
{
  ZMapConfigStanza stanza ;

  stanza = g_new0(ZMapConfigStanzaStruct, 1) ;
  stanza->name = g_strdup(stanza_name) ;
  stanza->elements = NULL ;

  return stanza ;
}


/* Copy a stanza. */
ZMapConfigStanza zmapConfigCopyStanza(ZMapConfigStanza stanza_in)
{
  ZMapConfigStanza new_stanza ;
  GList *element_list ;

  new_stanza = zmapConfigCreateStanza(stanza_in->name) ;

  /* Copy any elements if there are any. */
  if ((element_list = stanza_in->elements))
    {
      do
	{
	  ZMapConfigStanzaElement new_element ;

	  new_element = zmapConfigCopyElement(element_list->data) ;

	  new_stanza->elements = g_list_append(new_stanza->elements, new_element) ;
	}
      while ((element_list = g_list_next(element_list))) ;
    }

  return new_stanza ;
}



/* Free a stanza struct and its elements. */
void zmapConfigDestroyStanza(ZMapConfigStanza stanza)
{
  /* Free the elements. */
  if (stanza->elements)
    g_list_foreach(stanza->elements, glistFreeElement, NULL ) ;

  /* Free per stanza stuff... */
  g_free(stanza->name) ;
  g_free(stanza) ;

  return ;
}



/* This is a Glib GFunc function and is called for every element of the stanza GList of elements,
 * it in turn calls the routine to actually free the element. */
static void glistFreeElement(gpointer data, gpointer user_data_unused)
{
  ZMapConfigStanzaElement element = (ZMapConfigStanzaElement)data ;

  zmapConfigDestroyElement(element) ;

  return ;
}



ZMapConfigStanzaElement zmapConfigCreateElement(char *name, ZMapConfigElementType type)
{
  ZMapConfigStanzaElement element ;

  element = g_new0(ZMapConfigStanzaElementStruct, 1) ;
  element->name = g_strdup(name) ;
  element->type = type ;

  return element ;
}


/* Copy a stanza element. */
ZMapConfigStanzaElement zmapConfigCopyElement(ZMapConfigStanzaElement element_in)
{
  ZMapConfigStanzaElement new_element ;

  new_element = zmapConfigCreateElement(element_in->name, element_in->type) ;

  zmapConfigCopyElementData(element_in, new_element) ;

  return new_element ;
}


/* Copy data from one element to another. */
void zmapConfigCopyElementData(ZMapConfigStanzaElement element_in,
			       ZMapConfigStanzaElement element_out)
{
  switch(element_in->type)
    {
    case ZMAPCONFIG_BOOL:
      element_out->data.b = element_in->data.b ;
      break ;
    case ZMAPCONFIG_INT:
      element_out->data.i = element_in->data.i ;
      break ;
    case ZMAPCONFIG_FLOAT:
      element_out->data.f = element_in->data.f ;
      break ;
    case ZMAPCONFIG_STRING:
      element_out->data.s = g_strdup(element_in->data.s) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  return ;
}


void zmapConfigDestroyElement(ZMapConfigStanzaElement element)
{
  g_free(element->name) ;
  if (element->type == ZMAPCONFIG_STRING)
    g_free(element->data.s) ;
  g_free(element) ;

  return ;
}


