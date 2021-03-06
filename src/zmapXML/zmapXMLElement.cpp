/*  File: zmapXMLElement.c
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






#include <string.h>
#include <zmapXML_P.hpp>

static void markElementDirty(gpointer data, gpointer unused);
static void markAttributeDirty(gpointer item, gpointer data);
static void freeElement(gpointer data, gpointer unused);
static void freeAttributeListItems(gpointer item, gpointer data);

/* Creates a named element.  That's it.  */
ZMapXMLElement zmapXMLElementCreate(const XML_Char *name)
{
  ZMapXMLElement ele = NULL;
  int len   = 100;
  ele       = g_new0(zmapXMLElementStruct, 1);
  ele->name = g_quark_from_string(g_ascii_strdown((char *)name, -1));

  ele->contents = g_string_sized_new(len);

  return ele;
}

gboolean zmapXMLElementAddAttribute(ZMapXMLElement ele, ZMapXMLAttribute attr)
{
  ele->attributes = g_list_prepend(ele->attributes, attr);
  return TRUE;
}
gboolean zmapXMLElementAddAttrPair(ZMapXMLElement ele,
                                   const XML_Char *name,
                                   const XML_Char *value)
{
  ZMapXMLAttribute attr;
  attr = zmapXMLAttributeCreate(name, value);
  return zmapXMLElementAddAttribute(ele, attr);
}

void zmapXMLElementAddChild(ZMapXMLElement parent, ZMapXMLElement child)
{
  parent->children = g_list_insert(parent->children, child, -1);
  child->parent    = parent;
  return ;
}



/* Remove child from its parent. Note parent can legitimately be NULL if child
 * is the root e.g. if no tree ended up being built. */
gboolean zmapXMLElementSignalParentChildFree(ZMapXMLElement child)
{
  gboolean result = FALSE ;
  ZMapXMLElement parent = child->parent ;

  if (parent != NULL)
    {
      parent->children = g_list_remove(parent->children, child) ;

      result = TRUE ;
    }

  return result ;
}

void zmapXMLElementAddContent(ZMapXMLElement ele,
                              const XML_Char *content,
                              int len)
{
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
ZMapXMLElement zMapXMLElementNextSibling(ZMapXMLElement ele)
{
  GList *sibling, *current;

  current = g_list_find(ele->parent->children, ele);
  sibling = g_list_next(current);

  return (sibling ? (ZMapXMLElement)(sibling->data) : NULL);
}

ZMapXMLElement zMapXMLElementPreviousSibling(ZMapXMLElement ele)
{
  GList *sibling, *current;

  current = g_list_find(ele->parent->children, ele);
  sibling = g_list_previous(current);

  return (sibling ? (ZMapXMLElement)(sibling->data) : NULL);
}

/* This can only get first Child path. Probably not commonly used, but
 * here in case.
 */
ZMapXMLElement zMapXMLElementGetChildByPath(ZMapXMLElement parent, const char *path)
{
  ZMapXMLElement ele = NULL, pele = NULL;
  gchar **names = NULL;
  gboolean exist = TRUE;
  names = g_strsplit(path, ".", -1);
  int i = 0;
  pele = parent;
  while(exist)
    {
      if(names[i] == NULL)
        break;                  /* We're finished */
      if(strlen(names[i]) == 0)
        {                       /* path looked like 'elementa..elementb' */
          i++;
          continue;
        }
      if((ele = zMapXMLElementGetChildByName(pele, names[i])) == NULL)
        exist = FALSE;          /* Search failed, ele = NULL */
      pele = ele;
      i++;
    }

  return ele;
}

ZMapXMLElement zMapXMLElementGetChildByName(ZMapXMLElement parent, const char *name)
{
  if(name && *name)
    return zMapXMLElementGetChildByName1(parent, g_quark_from_string(g_ascii_strdown(name, -1)));
  else
    return NULL;
}

/* These two could provide use when converting the tree to users'
 * objects and basically allow slow hash style access to the GList
 * which is implemented as a list for the reason mentioned below.
 */
ZMapXMLElement zMapXMLElementGetChildByName1(ZMapXMLElement parent, GQuark name)
{
  GList *glist;

  glist = zMapXMLElementGetChildrenByName(parent, name, 1);

  return (glist ? (ZMapXMLElement)(glist->data) : NULL);
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
GList *zMapXMLElementGetChildrenByName(ZMapXMLElement parent, GQuark name, int expect)
{
  GList *item     = g_list_first(parent->children);
  GList *children = NULL;
  while(item)
    {
      ZMapXMLElement child;
      child = (ZMapXMLElement)(item->data);

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

ZMapXMLAttribute zMapXMLElementGetAttributeByName(ZMapXMLElement ele, const char *name)
{
  ZMapXMLAttribute attr = NULL;
  GList *item = g_list_first(ele->attributes);
  GQuark want = g_quark_from_string(g_ascii_strdown(name, -1));

  while(item)
    {
      ZMapXMLAttribute cur = (ZMapXMLAttribute)item->data;

      if(want == cur->name)
        {
          attr = cur;
          break;
        }

      item = g_list_next(item);
    }

  return attr;
}

char *zMapXMLElementStealContent(ZMapXMLElement element)
{
  char *steal = NULL;

  if(element->contents)
    {
      steal = element->contents->str;
      element->contents_stolen = TRUE;
    }

  return steal;
}

/* Frees an element and all that it contains (children and all) */
/*
     GQuark   name;
     GList   *children;
     GString *contents;
     GList   *attributes;
     zmapXMLElement parent;
*/
void zmapXMLElementFree(ZMapXMLElement ele)
{
  if(ele == NULL)
    return ;

  /* Free all my children */
  g_list_foreach(ele->children, freeElement, NULL);
  g_list_free(ele->children);

  /* And free myself... First the contents */
  if(ele->contents)
    g_string_free(ele->contents, (!(ele->contents_stolen)));

  /* Now the attributes */
  zmapXMLElementFreeAttrList(ele);

  /* my parent, doesn't need 2 b freed so set to null! */
  /* I might want a handler here, but not sure
   * (childFreedHandler)(ele->parent, ele);
   */
  ele->parent   = NULL;
  ele->children = NULL;
  ele->contents = NULL;

  /* Finally empty the ele. */
  g_free(ele);

  return ;
}

void zmapXMLElementMarkDirty(ZMapXMLElement ele)
{
  if(ele == NULL)
    return ;

  ele->dirty = TRUE;

  /* Free all my children */
  g_list_foreach(ele->children, markElementDirty, NULL);
  g_list_free(ele->children);

  if(ele->contents)
    g_string_free(ele->contents, (!(ele->contents_stolen)));

  /* Now the attributes */
  zmapXMLElementMarkAttributesDirty(ele);

  ele->parent   = NULL;
  ele->children = NULL;
  ele->contents = NULL;

  return ;
}

void zmapXMLElementMarkAttributesDirty(ZMapXMLElement ele)
{
  GList *glist = NULL;
  glist = ele->attributes;

  g_list_foreach(glist, markAttributeDirty, NULL);
  g_list_free(glist);

  ele->attributes = NULL;

  return ;
}
void zmapXMLElementFreeAttrList(ZMapXMLElement ele)
{
  GList *glist = NULL;
  glist = ele->attributes;

  g_list_foreach(glist, freeAttributeListItems, NULL);
  g_list_free(glist);

  ele->attributes = NULL;

  return ;
}




/* INTERNAL! */

static void freeElement(gpointer data, gpointer unused)
{
  ZMapXMLElement ele = (ZMapXMLElement)data;
  zmapXMLElementFree(ele);
  return ;
}
static void freeAttributeListItems(gpointer item, gpointer data)
{
  ZMapXMLAttribute attr = (ZMapXMLAttribute)item;

  zmapXMLAttributeFree(attr);

  return ;
}

static void markElementDirty(gpointer data, gpointer unused)
{
  ZMapXMLElement ele = (ZMapXMLElement)data;
  zmapXMLElementMarkDirty(ele);
  return ;
}

static void markAttributeDirty(gpointer item, gpointer data)
{
  ZMapXMLAttribute attr = (ZMapXMLAttribute)item;

  zmapXMLAttributeMarkDirty(attr);

  return ;
}

