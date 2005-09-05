/*  File: zmapXMLElement.c
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
 * Last edited: Sep  2 17:02 2005 (rds)
 * Created: Fri Aug  5 14:33:49 2005 (rds)
 * CVS info:   $Id: zmapXMLElement.c,v 1.1 2005-09-05 17:28:22 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>

static void freeElement(gpointer data, gpointer unused);

/* Creates a named element.  That's it.  */
zmapXMLElement zmapXMLElement_create(const XML_Char *name)
{
  zmapXMLElement ele = NULL;

  ele       = g_new0(zmapXMLElementStruct, 1);
  ele->name = g_quark_from_string(g_ascii_strdown((char *)name, -1));

  return ele;
}

gboolean zmapXMLElement_addAttribute(zmapXMLElement ele, zmapXMLAttribute attr)
{
  ele->attributes = g_list_prepend(ele->attributes, attr);
  return TRUE;
}
gboolean zmapXMLElement_add_attr_pair(zmapXMLElement ele,
                                      const XML_Char *name,
                                      const XML_Char *value)
{
  zmapXMLAttribute attr;
  attr = zmapXMLAttribute_create(name, value);
  return zmapXMLElement_addAttribute(ele, attr);
}

void zmapXMLElement_addChild(zmapXMLElement parent, zmapXMLElement child)
{
  parent->children = g_list_insert(parent->children, child, -1);
  child->parent    = parent;
  return ;
}
void zmapXMLElement_removeChild(zmapXMLElement parent, zmapXMLElement child)
{
  /* In case we get passed the root element as the the child, it's
     parent is NULL! */
  if(parent == NULL)
    return ;                    /* NOT SURE THIS IS RIGHT!  */
  parent->children = g_list_remove(parent->children, child);
  return ;
}

void zmapXMLElement_add_content(zmapXMLElement ele, 
                                const XML_Char *content,
                                int len)
{
  if(ele->contents == NULL)
    ele->contents = g_string_sized_new(2 * len);

  /* N.B. we _only_ get away with this because XML_Char == char at the moment, 
   * this will need to be fixed in the future to support UNICODE/UTF-16 etc...
   * See expat docs & expat_external.h for further info. SCEW's str.h uses
   * macros to use the correct type and we should do the same to convert from 
   * unicode if and when required....
   */
  ele->contents = g_string_append_len(ele->contents, 
                                      (char *)content, 
                                      len
                                      );
  return ;
}
/* These two are more for convenience, the use should probably be
 * interacting with the GList of children themselves
 */
/* Will eventually return NULL! caller should check! */
zmapXMLElement zMapXMLElement_nextSibling(zmapXMLElement ele)
{
  GList *sibling, *current;

  current = g_list_find(ele->parent->children, ele);
  sibling = g_list_next(current);
      
  return (sibling ? sibling->data : NULL);
}

zmapXMLElement zMapXMLElement_previousSibling(zmapXMLElement ele)
{
  GList *sibling, *current;

  current = g_list_find(ele->parent->children, ele);
  sibling = g_list_previous(current);
      
  return (sibling ? sibling->data : NULL);
}


/* These two could provide use when converting the tree to users'
 * objects and basically allow slow hash style access to the GList 
 * which is implemented as a list for the reason mentioned below.
 */
zmapXMLElement zMapXMLElement_getChildByName(zmapXMLElement parent,
                                             GQuark name)
{
  GList *list;

  list = zMapXMLElement_getChildrenByName(parent, name, 1);

  return (list ? list->data : NULL);
}

/* Exists to help above, but mainly as the following is possible in xml
 * <alpha>
 *   <beta>Lorem Ipsum Dolor</beta>
 *   <beta>Lorem Ipsum Dolor sit amet</beta>
 *   <gamma lorem="Ipsum"/>
 * </alpha>
 * glib's GList will only return the first one it finds.  Passing -1 as 
 * the expect is guarenteed to return all child elements with name.
 */
GList *zMapXMLElement_getChildrenByName(zmapXMLElement parent,
                                        GQuark name,
                                        int expect)
{
  GList *item     = g_list_first(parent->children);
  GList *children = NULL;
  while(item)
    {
      zmapXMLElement child;
      child = (zmapXMLElement)(item->data);

      if(child->name == name)
        {
          children = g_list_append(children, child);
          expect--;
        }

      if(expect != 0)
        item = g_list_next(item);  
      else
        break;
    }
  return children;
}

zmapXMLAttribute zMapXMLElement_getAttributeByName(zmapXMLElement ele,
                                                   char *name)
{
  zmapXMLAttribute attr = NULL;
  GList *item = g_list_first(ele->attributes);
  GQuark want = g_quark_from_string(g_ascii_strdown(name, -1));
  while(item)
    {
      zmapXMLAttribute cur = (zmapXMLAttribute)item->data;
      if(want == cur->name)
        {
          attr = cur;
          break;
        }
      item = g_list_next(item);
    }
  return attr;
}


/* Frees an element and all that it contains (children and all) */
/*   
     GQuark   name;
     GList   *children;
     GString *contents; 
     GList   *attributes;   
     zmapXMLElement parent;
*/
void zmapXMLElement_free(zmapXMLElement ele)
{
  GList *c;

  if(ele == NULL)
    return ;

  /* Free all my children */
  g_list_foreach(ele->children, freeElement, NULL);
  g_list_free(ele->children);

  /* And free myself... First the contents */
  if(ele->contents)
    g_string_free(ele->contents, TRUE);
  
  /* Now the attributes */
  zmapXMLAttribute_list_free(ele->attributes);
  
  /* my parent, doesn't need 2 b freed so set to null! */
  /* I might want a handler here, but not sure 
   * (childFreedHandler)(ele->parent, ele);
   */
  ele->parent = NULL;

  /* Finally empty the ele. */
  g_free(ele);

  return ;
}





/* INTERNAL! */

static void freeElement(gpointer data, gpointer unused)
{
  zmapXMLElement ele = (zmapXMLElement)data;
  zmapXMLElement_free(ele);
  return ;
}
