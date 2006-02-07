/*  File: zmapXMLParse.c
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
 * Last edited: Feb  6 16:52 2006 (rds)
 * Created: Fri Aug  5 12:49:50 2005 (rds)
 * CVS info:   $Id: zmapXMLParse.c,v 1.6 2006-02-07 09:11:12 rds Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <expat.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapXML_P.h>

typedef struct _tagHandlerItemStruct
{
  GQuark id;
  ZMapXMLMarkupObjectHandler handler;
} tagHandlerItemStruct, *tagHandlerItem;

static void setupExpat(zmapXMLParser parser);
static void freeUpTheQueue(zmapXMLParser parser);
/* Expat handlers we set */
static void start_handler(void *userData, 
                          const char *el, 
                          const char **attr);
static void end_handler(void *userData, 
                        const char *el);
static void character_handler(void *userData,
                              const XML_Char *s, 
                              int len);
static void xmldecl_handler(void* data, 
                            XML_Char const* version, 
                            XML_Char const* encoding,
                            int standalone);
static ZMapXMLMarkupObjectHandler getObjHandler(zmapXMLElement element,
                                                GList *tagHandlerItemList);
/* End of Expat handlers */

/* So what does this do?
 * ---------------------

 * A zmapXMLParser wraps an expat parser so all the settings for the
 * parser are in the one place making it simpler to create, hold on
 * to, destroy an expat parser with the associated start and end
 * handlers in multiple instances.  There is also some extra leverage
 * in the way we parse.  Rather than creating custom objects straight
 * off we create zmapXMLElements which also have an interface.  This
 * obviously uses more RAMs but hopefully the shepherding is of good 
 * quality.  There's also a start and end "Markup Object" handlers to
 * be set which are called from the expat start/end handlers we 
 * register here.  With these it's possible to step through the 
 * zmapXMLElements which will likely just be sub trees of the document.
 * You can of course not register these extra handlers and wait until
 * the complete zmapXMLdocument has been parsed and iterate through the 
 * zmapXMLElements yourself.

 */

zmapXMLParser zMapXMLParser_create(void *user_data, gboolean validating, gboolean debug)
{
  zmapXMLParser parser = NULL;

  parser     = (zmapXMLParser)g_new0(zmapXMLParserStruct, 1) ;

  /* Handle arguments */
  validating         = FALSE;           /* IGNORE! We DO NOT validate */
  parser->validating = validating;
  parser->debug      = debug;
  parser->user_data  = user_data;
  /* End effect of arguments */

  if (!(parser->expat = XML_ParserCreate(NULL)))
    zMapLogFatal("%s", "XML_ParserCreate() failed.");

  setupExpat(parser);

  parser->elementStack = g_queue_new() ;
  parser->last_errmsg  = NULL ;

  /* We don't need these generally, but users will */
  parser->startMOHandler = NULL;
  parser->endMOHandler   = NULL;

  return parser ;
}

zmapXMLElement zMapXMLParser_getRoot(zmapXMLParser parser)
{
  zmapXMLElement root = NULL;
  if(parser->document)
    root = parser->document->root;
  return root;
}

#define BUFFER_SIZE 200
gboolean zMapXMLParser_parseFile(zmapXMLParser parser,
                                  FILE *file)
{
  int len = 0;
  int done = 0;
  char buffer[BUFFER_SIZE];
  
  while (!done)
    {
      len = fread(buffer, 1, BUFFER_SIZE, file);
      if (ferror(file))
        {
          if (parser->last_errmsg)
            g_free(parser->last_errmsg) ;
          parser->last_errmsg = g_strdup_printf("File Read error");
          return 0;
        }

      done = feof(file);
      if(zMapXMLParser_parseBuffer(parser, (void *)buffer, len) != TRUE)
        return 0;
    }
  
  return 1;
}

gboolean zMapXMLParser_parseBuffer(zmapXMLParser parser, 
                                    void *data, 
                                    int size)
{
  gboolean result = TRUE ;
  int isFinal ;
  isFinal = (size ? 0 : 1);

#ifdef NEW_FEATURE_NOT_ON_ALPHAS
  XML_ParsingStatus status;
  XML_GetParsingStatus(parser->expat, &status);

  if(status.parsing == XML_FINISHED)
    zMapXMLParser_reset(parser);
#endif

  if (XML_Parse(parser->expat, (char *)data, size, isFinal) != XML_STATUS_OK)
    {
      if (parser->last_errmsg)
        g_free(parser->last_errmsg) ;
      
      parser->last_errmsg = g_strdup_printf("zmapXMLParse - Parse error (line %d): %s\n",
                                            XML_GetCurrentLineNumber(parser->expat),
                                            XML_ErrorString(XML_GetErrorCode(parser->expat))) ;
      result = FALSE ;
    }
  /* Because XML_ParsingStatus XML_GetParsingStatus aren't on alphas! */
  if(isFinal)
    result = zMapXMLParser_reset(parser);

  return result ;
}

void zMapXMLParser_setMarkupObjectHandler(zmapXMLParser parser, 
                                          ZMapXMLMarkupObjectHandler start,
                                          ZMapXMLMarkupObjectHandler end)
{
  parser->startMOHandler = start ;
  parser->endMOHandler   = end   ;
  return ;
}

void zMapXMLParser_setMarkupObjectTagHandlers(zmapXMLParser parser,
                                              ZMapXMLObjTagFunctions starts,
                                              ZMapXMLObjTagFunctions ends)
{
  ZMapXMLObjTagFunctions p = NULL;

  zMapAssert(starts || ends);

  p = starts;
  while(p && p->element_name != NULL)
    {
      tagHandlerItem item = g_new0(tagHandlerItemStruct, 1);
      item->id      = g_quark_from_string(p->element_name);
      item->handler = p->handler;
      parser->startTagHandlers = g_list_prepend(parser->startTagHandlers, item);
      p++;
    }

  p = ends;
  while(p && p->element_name != NULL)
    {
      tagHandlerItem item = g_new0(tagHandlerItemStruct, 1);
      item->id      = g_quark_from_string(p->element_name);
      item->handler = p->handler;
      parser->endTagHandlers = g_list_prepend(parser->endTagHandlers, item);
      p++;
    }

  return ;
}

#ifdef RDS_CLOSED_FACTORY
void zMapXMLParser_useFactory(zmapXMLParser parser,
                              zmapXMLFactory factory)
{
  if(factory->isLite)
    return ;                    /* Lite factories can't be used! */

  /* The parser doesn't free any of the user data so just overwriting
     here is fine. */
  parser->user_data = factory;
  zMapXMLParser_setMarkupObjectHandler(parser,
                                       zmapXML_FactoryStartHandler,
                                       zmapXML_FactoryEndHandler);
  return ;
}
#endif
char *zMapXMLParser_lastErrorMsg(zmapXMLParser parser)
{
  return parser->last_errmsg;
}

gboolean zMapXMLParser_reset(zmapXMLParser parser)
{
  gboolean result = TRUE;

  printf("\n\n ++++ PARSER is being reset ++++ \n\n");

  /* Clean up our data structures */
#warning FIX THIS MEMORY LEAK
  /* Check out this memory leak. 
   * It'd be nice to force the user to clean up the document.
   * a call to zMapXMLDocument_destroy(doc) should go to its root
   * and call zMapXMLElement_destroy(root) which will mean everything
   * is free....
   */
  parser->document = NULL;
  freeUpTheQueue(parser);

  if((result = XML_ParserReset(parser->expat, NULL))) /* encoding as it was created */
    setupExpat(parser);
  else
    parser->last_errmsg = "Failed Resetting the parser.";

  /* The expat parser doesn't clean up the userdata it gets sent.
   * That's us anyway, so that's good, and we don't need to do anything here!
   */

  /* Do so reinitialising for our stuff. */
  parser->elementStack = g_queue_new() ;
  parser->last_errmsg  = NULL ;

  return result;
}

void zMapXMLParser_destroy(zmapXMLParser parser)
{
  XML_ParserFree(parser->expat) ;

  freeUpTheQueue(parser);

  if (parser->last_errmsg)
    g_free(parser->last_errmsg) ;

  parser->user_data = NULL;

  g_free(parser) ;

  return ;
}

/* INTERNAL */
static void setupExpat(zmapXMLParser parser)
{
  if(parser == NULL)
    return;

  XML_SetElementHandler(parser->expat, start_handler, end_handler);
  XML_SetCharacterDataHandler(parser->expat, character_handler);
  XML_SetXmlDeclHandler(parser->expat, xmldecl_handler);
  XML_SetUserData(parser->expat, (void *)parser);

  return ;
}

static void freeUpTheQueue(zmapXMLParser parser)
{
  if (!g_queue_is_empty(parser->elementStack))
    {
      gpointer dummy ;

      while ((dummy = g_queue_pop_head(parser->elementStack)))
	{
	  g_free(dummy) ;
	}
    }
  g_queue_free(parser->elementStack) ;
  parser->elementStack = NULL;
  /* Queue has gone */

  return ;
}

/* HANDLERS */

/* Set handlers for start and end tags. Attributes are passed to the start handler
 * as a pointer to a vector of char pointers. Each attribute seen in a start (or empty)
 * tag occupies 2 consecutive places in this vector: the attribute name followed by the
 * attribute value. These pairs are terminated by a null pointer. */

static void start_handler(void *userData, 
                          const char *el, 
                          const char **attr)
{
  zmapXMLParser parser = (zmapXMLParser)userData ;
  zmapXMLElement current_ele = NULL ;
  ZMapXMLMarkupObjectHandler handler = NULL;
  int depth, i;

#ifdef ZMAP_XML_PARSE_USE_DEPTH
  depth = parser->depth;
#else
  depth = (int)g_queue_get_length(parser->elementStack);
#endif

  if (parser->debug)
    {
      for (i = 0; i < depth; i++)
	printf("  ");
      
      printf("<%s ", el) ;

      for (i = 0; attr[i]; i += 2)
        printf(" %s='%s'", attr[i], attr[i + 1]);

      printf(">\n");
    }

  /* Push the current tag on to our stack of tags. */
  current_ele = zmapXMLElement_create(el);

  for(i = 0; attr[i]; i+=2){
    zmapXMLAttribute attribute = zmapXMLAttribute_create(attr[i], attr[i + 1]);
    zmapXMLElement_addAttribute(current_ele, attribute);
  }


  if(!depth)
    {
      /* In case there wasn't a <?xml version="1.0" ?> */
      if(!(parser->document))
        parser->document = zMapXMLDocument_create("1.0", "UTF-8", -1);
      zMapXMLDocument_setRoot(parser->document, current_ele);
    }
  else
    {
      zmapXMLElement parent_ele;
      parent_ele = g_queue_peek_head(parser->elementStack);
      zmapXMLElement_addChild(parent_ele, current_ele);
    }
    
  g_queue_push_head(parser->elementStack, current_ele);

#ifdef ZMAP_XML_PARSE_USE_DEPTH
  (parser->depth)++;
#endif

  if(((handler = parser->startMOHandler) != NULL) || 
     ((handler = getObjHandler(current_ele, parser->startTagHandlers)) != NULL))
     (*handler)((void *)parser->user_data, current_ele, parser);

  return ;
}


static void end_handler(void *userData, 
                        const char *el)
{
  zmapXMLParser parser = (zmapXMLParser)userData ;
  zmapXMLElement current_ele ;
  gpointer dummy ;
  ZMapXMLMarkupObjectHandler handler = NULL;
  int i ;

  /* Now get rid of head element. */
  dummy       = g_queue_pop_head(parser->elementStack) ;
  current_ele = (zmapXMLElement)dummy ;

  if (parser->debug)
    {
#ifdef ZMAP_XML_PARSE_USE_DEPTH
      int depth = parser->depth;
#else
      int depth = g_queue_get_length(parser->elementStack);
#endif
      for (i = 0; i < depth; i++)
	printf("  ") ;

      printf("</%s>\n", el) ;
    }
#ifdef ZMAP_XML_PARSE_USE_DEPTH
  (parser->depth)--;
#endif
  if(((handler = parser->endMOHandler) != NULL) || 
     ((handler = getObjHandler(current_ele, parser->endTagHandlers)) != NULL))
    {
      if(((*handler)((void *)parser->user_data, current_ele, parser)) == TRUE)
        {
          /* We can free the current_ele and all its children */
          /* First we need to tell the parent that its child is being freed. */
          if(!(zmapXMLElement_signalParentChildFree(current_ele)))
             printf("Empty'ing document... this might cure memory leak...\n ");
          zmapXMLElement_free(current_ele);
        }
    }
  return ;
}

/* Set a text handler. The string your handler receives is NOT zero terminated.
 * You have to use the length argument to deal with the end of the string.
 * A single block of contiguous text free of markup may still result in a
 * sequence of calls to this handler. In other words, if you're searching for a pattern
 * in the text, it may be split across calls to this handler. 

 * Best to delay anything that wants the whole string to the end handler.
 */
static void character_handler(void *userData, 
                              const XML_Char *s, 
                              int len)
{
  zmapXMLElement content4ele;
  zmapXMLParser parser = (zmapXMLParser)userData ; 

  content4ele = g_queue_peek_head(parser->elementStack);

  zmapXMLElement_addContent(content4ele, s, len);

  return ;
}

static void xmldecl_handler(void* data, 
                            XML_Char const* version, 
                            XML_Char const* encoding,
                            int standalone)
{
    zmapXMLParser parser = (zmapXMLParser)data;

    if (parser == NULL)
      return ;

    if (!(parser->document))
      parser->document = zMapXMLDocument_create(version, encoding, standalone);

    return ;
}

static ZMapXMLMarkupObjectHandler getObjHandler(zmapXMLElement element,
                                                GList *tagHandlerItemList)
{
  ZMapXMLMarkupObjectHandler handler = NULL;
  GList *list = NULL;
  GQuark id = 0;
  id   = element->name;
  list = tagHandlerItemList;

  while(list){
    tagHandlerItem item = (tagHandlerItem)(list->data);
    if(id == item->id){
      handler = item->handler;
      break;
    }
    list = list->next;
  }

  return handler;
}



#ifdef RDS_NEVER
  /* If there is content between the start/end tags process it. */
  if (current_ele->content->len)
    {
      if (parser->debug)
	{
          int depth = g_queue_get_length(parser->elementStack);
	  for (i = 0; i < depth; i++)
	    printf("  ");

	  printf("%s content -  %s\n", current_ele->element_name, parser->content->str) ;
	}

      /* Here is where we need to do some of the (most of the ?) gff processing.... */
      /* reset once processed..... */
      parser->content = g_string_set_size(parser->content, 0) ;
    }

  g_free(dummy) ;
#endif
