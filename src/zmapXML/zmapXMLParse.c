/*  File: zmapXMLParse.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Provides xml parsing, under the covers uses the expat
 *              parser.
 *
 * Exported functions: See ZMap/zmapXML.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdio.h>
#include <expat.h>
#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <zmapXML_P.h>



/* These limits should be removed...see comments at start of code. */
enum {PARSE_ELEMENT_LIMIT = 3000, PARSE_ATTRIBUTE_LIMIT = PARSE_ELEMENT_LIMIT * 2} ;


typedef struct _tagHandlerItemStruct
{
  GQuark id;
  ZMapXMLMarkupObjectHandler handler;
} tagHandlerItemStruct, *tagHandlerItem;



static void setupExpat(ZMapXMLParser parser);
static void initElements(GArray *array);
static void initAttributes(GArray *array);
static void freeUpTheQueue(ZMapXMLParser parser);
static void freeTagHandlers(gpointer data, gpointer un_used_data);
static char *getOffendingXML(ZMapXMLParser parser, int context);
static void abortParsing(ZMapXMLParser parser, char *reason, ...);
static ZMapXMLElement parserFetchNewElement(ZMapXMLParser parser, 
                                            const XML_Char *name);
static ZMapXMLAttribute parserFetchNewAttribute(ZMapXMLParser parser,
                                                const XML_Char *name,
                                                const XML_Char *value);
static void pushXMLBase(ZMapXMLParser parser, const char *xmlBase);
/* A "user" level ZMapXMLMarkupObjectHandler to handle removing xml bases. */
static gboolean popXMLBase(void *userData, 
                           ZMapXMLElement element, 
                           ZMapXMLParser parser);

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
/* End of Expat handlers */

static ZMapXMLMarkupObjectHandler getObjHandler(ZMapXMLElement element,
                                                GList *tagHandlerItemList);
static void setupAutomagicCleanup(ZMapXMLParser parser, ZMapXMLElement element);
static gboolean defaultAlwaysTrueHandler(void *ignored_data,
                                         ZMapXMLElement element,
                                         ZMapXMLParser parser);





/* OK....SO THIS LIMIT IS FUNDAMENTALLY A BAD IDEA.....I'VE BUMPED IT UP
 * AS AN INITIAL FIX BUT IT NEEDS FIXING PROPERLY..... SEE ENUM ABOVE
 * 
 * NOTE...YOU _CANNOT_ FIX THIS PROBLEM JUST SIMPLY BY INCREASING THE SIZE
 * OF THE ATTRIBUTE AND ELEMENT ARRAYS DYNAMICALLY BECAUSE OTHER CODE HOLDS
 * POINTERS INTO THESE ARRAYS WHICH BECOME INVALID WHEN THE ARRAY IS RELOCATED
 * DURING THE RESIZE. SEE RT 59402
 * 
 * 
 * 
 * What happens of course is that sooner or later we exceed the limit and then
 * the code crashes..... */

/* So what does this do?
 * ---------------------
 *
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
 * zmapXMLElements yourself. Recommended ONLY if document is small 
 * (< 100 elements, < 200 attributes).
 *
 * I've imposed this limit to make memory use small.
 *
 * size of object is dependent on the depth of the object. e.g a list
 * can be 99 elements in size if it's only contained, by one other.  But
 * if it's 10 levels deep the limit is 90
 *
 */

ZMapXMLParser zMapXMLParserCreate(void *user_data, gboolean validating, gboolean debug)
{
  ZMapXMLParser parser = NULL;

  parser     = (ZMapXMLParser)g_new0(zmapXMLParserStruct, 1) ;

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

  /* This should probably be done with a NS parser */
  parser->xmlbase = g_quark_from_string(ZMAP_XML_BASE_ATTR);

  parser->max_size_e = PARSE_ELEMENT_LIMIT ;
  parser->max_size_a =  PARSE_ATTRIBUTE_LIMIT ;
  parser->elements   = g_array_sized_new(TRUE, TRUE, sizeof(zmapXMLElementStruct),   parser->max_size_e) ;
  parser->attributes = g_array_sized_new(TRUE, TRUE, sizeof(zmapXMLAttributeStruct), parser->max_size_a) ;

  g_array_set_size(parser->elements, parser->max_size_e);
  g_array_set_size(parser->attributes, parser->max_size_a);

  initElements(parser->elements);
  initAttributes(parser->attributes);

  return parser ;
}

void zMapXMLParserSetUserData(ZMapXMLParser parser, void *user_data)
{
  if (!parser)
    return ;
  parser->user_data = user_data;

  return ;
}

ZMapXMLElement zMapXMLParserGetRoot(ZMapXMLParser parser)
{
  ZMapXMLElement root = NULL;

  if(parser->document)
    root = parser->document->root;

  return root;
}

char *zMapXMLParserGetBase(ZMapXMLParser parser)
{
  char *base = NULL;

  if(parser->expat != NULL)
    base = (char *)XML_GetBase(parser->expat);

  return base;
}



long zMapXMLParserGetCurrentByteIndex(ZMapXMLParser parser)
{
  long index = 0;

  if(parser->expat != NULL)
    index = XML_GetCurrentByteIndex(parser->expat);

  return index;
}


/* If return is non null it needs freeing sometime in the future! */
char *zMapXMLParserGetFullXMLTwig(ZMapXMLParser parser, int offset)
{
  char *copy = NULL, *tmp1 = NULL;
  const char *tmp = NULL;
  int current = 0, size = 0, byteCount = 0, twigSize = 0;
  if((tmp = XML_GetInputContext(parser->expat, &current, &size)) != NULL)
    {
      byteCount = XML_GetCurrentByteCount(parser->expat);
      if(byteCount && current && size &&
         (twigSize = (int)((byteCount + current) - offset + 1)) <= size)
        {
          tmp1 = (char *)(tmp + offset);
          copy = g_strndup(tmp1, twigSize);
        }
    }

  return copy;                  /* PLEASE free me later */
}



gboolean zMapXMLParserParseFile(ZMapXMLParser parser,
				FILE *file)
{
  int len = 0;
  int done = 0;
  enum {BUFFER_SIZE = 200} ;
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
      if(zMapXMLParserParseBuffer(parser, (void *)buffer, len) != TRUE)
        return 0;
    }
  
  return 1;
}

static gboolean parse_xml(XML_Parser expat, char *buffer, int size, enum XML_Status *status_out)
{
  enum XML_Status processing_status;
  int is_final = (size ? 0 : 1);

  gboolean suspended = FALSE;

  if(status_out && *status_out != XML_STATUS_ERROR)
    {
      processing_status = XML_Parse(expat, buffer, size, is_final);
      
      switch(processing_status)
	{
	case XML_STATUS_ERROR:
	  suspended = FALSE;
	  break;
	case XML_STATUS_SUSPENDED:
	  suspended = TRUE;
	  break;
	default:
	  suspended = FALSE;
	  break;
	}

      *status_out = processing_status;
    }

  return suspended;
}

static gboolean resume_parse_xml(XML_Parser expat, char *buffer, 
				 int size, enum XML_Status *status_out)
{
  int suspended = 0;

  *status_out = XML_ResumeParser(expat);

  switch(*status_out)
    {
    case XML_STATUS_ERROR:
      suspended = 0;
      break;
    case XML_ERROR_NOT_SUSPENDED:
      suspended = 0;
      break;
    case XML_STATUS_SUSPENDED:
      suspended = 1;
      break;
    default:
      suspended = parse_xml(expat, buffer, size, status_out);
      break;
    }

  return suspended;
}

static void xml_parse(ZMapXMLParser parser, char *buffer, int size, enum XML_Status *status_out)
{
  enum XML_Status tmp_status = XML_STATUS_ERROR, *status_out_cp;
  int suspended = 0;

  if(status_out)
    status_out_cp = status_out;
  else
    status_out_cp = &tmp_status;

  if((suspended = parse_xml(parser->expat, buffer, size, status_out_cp)))
    {
      int offset = 0, c_size;

      while(suspended)
	{
	  const char *c;
	  gboolean ready_to_resume = TRUE;

	  if((parser->suspended_cb))
	    ready_to_resume = (parser->suspended_cb)(parser->user_data, parser);
	    
	  c = XML_GetInputContext(parser->expat, &offset, &c_size);

	  if(ready_to_resume)
	    suspended = resume_parse_xml(parser->expat, 
					 buffer + c_size, 
					 size   - c_size, 
					 status_out_cp);
	}
    }
  
  return ;
}

gboolean zMapXMLParserParseBuffer(ZMapXMLParser parser, 
				  void *data, 
				  int size)
{
  gboolean result = TRUE ;
  int isFinal;
  enum XML_Status processing_status = XML_STATUS_SUSPENDED;
  isFinal = (size ? 0 : 1);

#define ZMAP_USING_EXPAT_1_95_8_OR_ABOVE
#ifdef ZMAP_USING_EXPAT_1_95_8_OR_ABOVE
  XML_ParsingStatus status;
  XML_GetParsingStatus(parser->expat, &status);

  if(status.parsing == XML_FINISHED)
    zMapXMLParserReset(parser);
#endif

  xml_parse(parser, (char *)data, size, &processing_status);
  if (processing_status != XML_STATUS_OK)

#ifdef NEW_STYLE
    if ((processing_status = XML_Parse(parser->expat, (char *)data, size, isFinal)) != XML_STATUS_OK)
#endif
      {
	enum XML_Error error;
	char *offend = NULL;

	if (parser->last_errmsg)
	  g_free(parser->last_errmsg);

	error = XML_GetErrorCode(parser->expat);
      
	if(error == XML_ERROR_ABORTED && parser->error_free_abort)
	  result = TRUE;
	else
	  {                       /* processing_status == XML_STATUS_ERROR */
	    int line_num, col_num;

	    line_num = (int)XML_GetCurrentLineNumber(parser->expat);
	    col_num  = (int)XML_GetCurrentColumnNumber(parser->expat);

	    offend = getOffendingXML(parser, ZMAP_XML_ERROR_CONTEXT_SIZE);
	    switch(error)
	      {
	      case XML_ERROR_ABORTED:
		parser->last_errmsg = g_strdup_printf("[ZMapXMLParse] Parse error line %d column %d\n"
						      "[ZMapXMLParse] Aborted: %s\n",
						      line_num, col_num,
						      parser->aborted_msg);
		break;
	      default:
		parser->last_errmsg = g_strdup_printf("[ZMapXMLParse] Parse error line %d column %d\n"
						      "[ZMapXMLParse] Expat reports: %s\n"
						      "[ZMapXMLParse] XML near error <!-- >>>>%s<<<< -->\n",
						      line_num, col_num,
						      XML_ErrorString(error),
						      offend) ;
		break;
	      }
	    result = FALSE ;
	  }

	if(offend)
	  g_free(offend);
      }

#ifndef ZMAP_USING_EXPAT_1_95_8_OR_ABOVE
  /* Because XML_ParsingStatus XML_GetParsingStatus aren't on alphas! */
  if(isFinal)
    result = zMapXMLParserReset(parser);
#endif

  return result ;
}

void zMapXMLParserSetMarkupObjectHandler(ZMapXMLParser parser, 
                                         ZMapXMLMarkupObjectHandler start,
                                         ZMapXMLMarkupObjectHandler end)
{
  parser->startMOHandler = start ;
  parser->endMOHandler   = end   ;
  return ;
}

void zMapXMLParserSetMarkupObjectTagHandlers(ZMapXMLParser parser,
                                             ZMapXMLObjTagFunctions starts,
                                             ZMapXMLObjTagFunctions ends)
{
  ZMapXMLObjTagFunctions p = NULL;

  if (!(starts || ends))
    return ;

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

char *zMapXMLParserLastErrorMsg(ZMapXMLParser parser)
{
  return parser->last_errmsg;
}

void zMapXMLParserRaiseParsingError(ZMapXMLParser parser,
                                    char *error_string)
{
  parser->error_free_abort = FALSE;
  abortParsing(parser, "%s", error_string);
  return ;
}

static gboolean default_parser_suspended_cb(gpointer user_data, ZMapXMLParser parser)
{
  return TRUE;
}

void zMapXMLParserPauseParsing(ZMapXMLParser parser)
{
  /* need to check if in an active parser? */
  if((XML_StopParser(parser->expat, TRUE)) == XML_STATUS_OK)
    {
      /* do something to get the current position. */
      parser->suspended_cb = default_parser_suspended_cb;
    }
  else
    {
      /* Set an error message... */
    }
  
  return ;
}

/* Prematurely stop parsing, without creating an error. */
void zMapXMLParserStopParsing(ZMapXMLParser parser)
{
  parser->error_free_abort = TRUE;
  abortParsing(parser, "%s", "zMapXMLParserStopParsing called.");
  return ;
}

gboolean zMapXMLParserReset(ZMapXMLParser parser)
{
  gboolean result = TRUE;

  if(parser->debug)
    printf("\n ++++ PARSER is being reset ++++ \n");

  /* Clean up our data structures */
  /* Check out this memory leak. 
   * It'd be nice to force the user to clean up the document.
   * a call to zMapXMLDocument_destroy(doc) should go to its root
   * and call zMapXMLElement_destroy(root) which will mean everything
   * is free....
   */
  parser->document = NULL;

  parser->startMOHandler = 
    parser->endMOHandler = NULL;

  if(parser->startTagHandlers)
    {
      g_list_foreach(parser->startTagHandlers, freeTagHandlers, NULL);
      g_list_free(parser->startTagHandlers);
    }
  if(parser->endTagHandlers)
    {
      g_list_foreach(parser->endTagHandlers, freeTagHandlers, NULL);
      g_list_free(parser->endTagHandlers);
    }

  parser->startTagHandlers = 
    parser->endTagHandlers = NULL;

  freeUpTheQueue(parser);

  if((result = XML_ParserReset(parser->expat, NULL))) /* encoding as it was created */
    setupExpat(parser);
  else
    parser->last_errmsg = "[ZMapXMLParse] Failed Resetting the parser.";

  /* The expat parser doesn't clean up the userdata it gets sent.
   * That's us anyway, so that's good, and we don't need to do anything here!
   */

  /* Do so reinitialising for our stuff. */
  parser->elementStack = g_queue_new() ;
  parser->last_errmsg  = NULL ;

  initElements(parser->elements);
  initAttributes(parser->attributes);

  return result;
}

void zMapXMLParserDestroy(ZMapXMLParser parser)
{
  XML_ParserFree(parser->expat) ;

  freeUpTheQueue(parser);

  if (parser->last_errmsg)
    g_free(parser->last_errmsg) ;
  if (parser->aborted_msg)
    g_free(parser->aborted_msg) ;

  parser->user_data = NULL;

  /* We need to free the GArrays here!!! */

  g_array_free(parser->attributes, TRUE);
  g_array_free(parser->elements,   TRUE);

  g_free(parser) ;

  return ;
}

/* INTERNAL */
static void setupExpat(ZMapXMLParser parser)
{
  if(parser == NULL)
    return;

  XML_SetElementHandler(parser->expat, start_handler, end_handler);
  XML_SetCharacterDataHandler(parser->expat, character_handler);
  XML_SetXmlDeclHandler(parser->expat, xmldecl_handler);
  XML_SetUserData(parser->expat, (void *)parser);

  return ;
}

static void freeUpTheQueue(ZMapXMLParser parser)
{
  if (!g_queue_is_empty(parser->elementStack))
    {
      gpointer dummy ;
      /* elements are allocated/deallocated elsewhere now. */
      while ((dummy = g_queue_pop_head(parser->elementStack)))
	{
          /* g_free(dummy) ; */
	}
    }
  g_queue_free(parser->elementStack) ;
  parser->elementStack = NULL;
  /* Queue has gone */

  return ;
}

static void freeTagHandlers(gpointer data, gpointer un_used_data)
{
  tagHandlerItem to_free = (tagHandlerItem)data;
  if(to_free)
    {
      to_free->id = 0;
      to_free->handler = NULL;
      g_free(to_free);
    }
  return ;
}
static void pushXMLBase(ZMapXMLParser parser, const char *xmlBase)
{
  /* Add to internal list of stuff */
  parser->xmlBaseStack = g_list_append(parser->xmlBaseStack, (char *)xmlBase);

  XML_SetBase(parser->expat, xmlBase);

  return ;
}

/* N.B. element passed in is NULL! */
static gboolean popXMLBase(void *userData, 
                           ZMapXMLElement element, 
                           ZMapXMLParser parser)
{
  char *previousXMLBase = NULL, *current = NULL;

  /* Remove from list.  Is this correct? */
  if((current = zMapXMLParserGetBase(parser)) != NULL)
    {
      parser->xmlBaseStack = g_list_remove(parser->xmlBaseStack, current);
      
      previousXMLBase = (char *)((g_list_last(parser->xmlBaseStack))->data);
    }

  XML_SetBase(parser->expat, previousXMLBase);

  return FALSE;                   /* Ignored anyway! */
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
  ZMapXMLParser parser = (ZMapXMLParser)userData ;
  ZMapXMLElement current_ele = NULL ;
  ZMapXMLAttribute attribute = NULL ;
  ZMapXMLMarkupObjectHandler handler = NULL;
  int depth, i;
  gboolean currentHasXMLBase = FALSE, good = TRUE;

#ifdef ZMAP_XML_PARSE_USE_DEPTH
  depth = parser->depth;
#else
  depth = (int)g_queue_get_length(parser->elementStack);
#endif

  if (parser->debug)
    {
      for (i = 0; i < depth; i++)
        printf(" ");
      printf("<%s ", el) ;
      for (i = 0; attr[i]; i += 2)
        printf(" %s='%s'", attr[i], attr[i + 1]);
      printf(">");
    }

#ifdef RDS_DONT_INCLUDE
  current_ele = zmapXMLElementCreate(el);
#endif
  
  if((current_ele = parserFetchNewElement(parser, el)) == NULL)
    good = FALSE;
  else
    {
      for(i = 0; attr[i]; i+=2){
#ifdef RDS_DONT_INCLUDE
        attribute = ZMapXMLAttribute_create(attr[i], attr[i + 1]);
#endif
        if(good && (attribute = parserFetchNewAttribute(parser, attr[i], attr[i+1])) != NULL)
          {
            zmapXMLElementAddAttribute(current_ele, attribute);
            if(attribute->name == parser->xmlbase)
              parser->useXMLBase = currentHasXMLBase = TRUE;
          }
        else{ good = FALSE; break; }
      }
    }

  /* We've likely run out of elements or attributes. Parser will stop too! */
  if(!good)                     
    return ;                    /* Good to breakpoint this line */
  /* ^^^ *** EARLY RETURN *** */

  if(!depth)
    {
      /* In case there wasn't a <?xml version="1.0" ?> */
      if(!(parser->document))
        parser->document = zMapXMLDocumentCreate("1.0", "UTF-8", -1);
      zMapXMLDocumentSetRoot(parser->document, current_ele);
      setupAutomagicCleanup(parser, current_ele);
    }
  else
    {
      ZMapXMLElement parent_ele;
      parent_ele = g_queue_peek_head(parser->elementStack);
      zmapXMLElementAddChild(parent_ele, current_ele);
    }
    
  g_queue_push_head(parser->elementStack, current_ele);

#ifdef ZMAP_XML_PARSE_USE_DEPTH
  (parser->depth)++;
#endif

#ifdef ZMAP_XML_VALIDATING_PARSER
  if(parser->validating && parser->schema)
    {
      gboolean element_valid = FALSE;
      if((element_valid = zMapXMLSchemaValidateElementPosition(parser->schema, current_ele)) == FALSE)
        abortParsing(parser, "Element '%s' had bad position.", el);
      if(element_valid && 
         (element_valid = zMapXMLSchemaValidateElementAttributes(parser->schema, current_ele)) == FALSE)
        abortParsing(parser, "Element '%s' had bad attributes.", el);
    }
#endif  

  if (parser->useXMLBase && currentHasXMLBase
      && ((attribute = zMapXMLElementGetAttributeByName(current_ele, ZMAP_XML_BASE_ATTR)) != NULL))
    {
      tagHandlerItem item = g_new0(tagHandlerItemStruct, 1);
      item->id      = current_ele->name;
      item->handler = popXMLBase;
      pushXMLBase(parser, g_quark_to_string(zMapXMLAttributeGetValue(attribute)));
      parser->xmlBaseHandlers = g_list_prepend(parser->xmlBaseHandlers, item);      
    }

  if (((handler = parser->startMOHandler) != NULL)
      || ((handler = getObjHandler(current_ele, parser->startTagHandlers)) != NULL))
    {
      /* THE HANDLER FUNCTION IS REQUIRED TO RETURN A GBOOLEAN BUT THE CODE HERE
       * JUST IGNORES THIS VALUE.....HOW RUBBISH IS THAT..... */

      (*handler)((void *)parser->user_data, current_ele, parser);
    }

  return ;
}


static void end_handler(void *userData, 
                        const char *el)
{
  ZMapXMLParser parser = (ZMapXMLParser)userData ;
  ZMapXMLElement current_ele ;
  gpointer dummy ;
  ZMapXMLMarkupObjectHandler handler = NULL, xmlBaseHandler = NULL;
  int i ;

  /* Now get rid of head element. */
  dummy       = g_queue_pop_head(parser->elementStack) ;
  current_ele = (ZMapXMLElement)dummy ;

  if (parser->debug)
    {
#ifdef ZMAP_XML_PARSE_USE_DEPTH
      int depth = parser->depth;
#else
      int depth = g_queue_get_length(parser->elementStack);
#endif
      for (i = 0; !(current_ele->contents->len) && i < depth; i++)
      	printf("  ") ;

      printf("</%s>", el) ;
    }
#ifdef ZMAP_XML_PARSE_USE_DEPTH
  (parser->depth)--;
#endif

  /* Need to get this BEFORE we possibly destroy the current_ele below */
  if(parser->useXMLBase)
    xmlBaseHandler = getObjHandler(current_ele, parser->xmlBaseHandlers);

  if(((handler = parser->endMOHandler) != NULL) || 
     ((handler = getObjHandler(current_ele, parser->endTagHandlers)) != NULL))
    {
      if(((*handler)((void *)parser->user_data, current_ele, parser)) == TRUE)
        {
          /* We can free the current_ele and all its children */
          /* First we need to tell the parent that its child is being freed. */
          if(!(zmapXMLElementSignalParentChildFree(current_ele)))
	    printf("[zmapXMLParser] XML Document free? Memory leak ?\n");

          zmapXMLElementMarkDirty(current_ele);
        }
      else if(parser->array_point_e == parser->max_size_e - 1)
        {
          /* If we get here, we're running out of allocated elements */
          printf("[zmapXMLParser] Possible information loss...\n");
          zmapXMLElementMarkDirty(current_ele);
        }
    }
  else if(parser->array_point_e == parser->max_size_e - 1)
    {
      /* If we get here, we're running out of allocated elements */
      /* To fix this requires an increase in the number of end_handlers 
       * which return TRUE */
      printf("[zmapXMLParser] Possible information loss...\n");
      zmapXMLElementMarkDirty(current_ele);
    }

  /* We need to do this AFTER the endTagHandler as the xml:base 
   * still applies in that handler! */
  if(xmlBaseHandler != NULL)
    (*xmlBaseHandler)((void *)parser->user_data, NULL, parser);

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
  ZMapXMLElement content4ele;
  ZMapXMLParser parser = (ZMapXMLParser)userData ; 

  content4ele = g_queue_peek_head(parser->elementStack);

  zmapXMLElementAddContent(content4ele, s, len);

  if(parser->debug)
    {
      char save, *s_ptr;

      s_ptr  = (char *)(s + len);
      save   = *s_ptr;
      *s_ptr = '\0';
      printf("%s", s);
      *s_ptr = save;
    }

  return ;
}

static void xmldecl_handler(void* data, 
                            XML_Char const* version, 
                            XML_Char const* encoding,
                            int standalone)
{
  ZMapXMLParser parser = (ZMapXMLParser)data;

  if (parser == NULL)
    return ;

  if (!(parser->document))
    parser->document = zMapXMLDocumentCreate(version, encoding, standalone);

  return ;
}

static ZMapXMLMarkupObjectHandler getObjHandler(ZMapXMLElement element,
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

/* Why am I not using g lib memory stuff here, or even handle package.
 * The main reason is I don't want the xml to consume huge amounts of
 * memory.  This wrapper is designed to make things easier, but has 
 * the potential for abuse.  The idea all along was to create small
 * twigs of 'objecty' type data as an intermediate for user objects.
 */
static ZMapXMLAttribute parserFetchNewAttribute(ZMapXMLParser parser,
                                                const XML_Char *name,
                                                const XML_Char *value)
{
  ZMapXMLAttribute attr = NULL;
  int i = 0, save = -1;

  if (!parser->attributes)
    return attr ;

  for (i = 0; i < parser->attributes->len; i++)
    {
      /* This is hideous. Ed has a written a zMap_g_array_index, but it auto expands  */
      if (((&(g_array_index(parser->attributes, zmapXMLAttributeStruct, i))))->dirty == TRUE)
	{
	  save = i;
	  break;
	}
    }

  if ((save != -1) && (attr = &(g_array_index(parser->attributes, zmapXMLAttributeStruct, save))) != NULL)
    {
      attr->dirty = FALSE;
      attr->name  = g_quark_from_string(g_ascii_strdown((char *)name, -1));
      attr->value = g_quark_from_string((char *)value);
    }

  if (attr == NULL)
    abortParsing(parser, "Exhausted allocated (%d) attributes.", parser->attributes->len) ;

  return attr ;
}

static ZMapXMLElement parserFetchNewElement(ZMapXMLParser parser, 
                                            const XML_Char *name)
{
  ZMapXMLElement element = NULL;
  int i = 0, save = -1;

  if (!parser->elements)
    return element ;

  for (i = 0; i < parser->elements->len; i++)
    {
      /* This is hideous. Ed has a written a zMap_g_array_index, but it auto expands  */
      if (((&(g_array_index(parser->elements, zmapXMLElementStruct, i))))->dirty == TRUE)
	{
	  save = i;
	  break;
	}
    }

  if((save != -1) && (element = &(g_array_index(parser->elements, zmapXMLElementStruct, save))) != NULL)
    {
      element->dirty    = FALSE;
      element->name     = g_quark_from_string(g_ascii_strdown((char *)name, -1));
      element->contents = g_string_sized_new(100);
      parser->array_point_e = save;
    }

  if(element == NULL)
    abortParsing(parser, "Exhausted allocated (%d) elements.", parser->elements->len);

  return element;
}

static void initElements(GArray *array)
{
  ZMapXMLElement element = NULL;
  int i;

  for(i = 0; i < array->len; i++)
    {
      element = &(g_array_index(array, zmapXMLElementStruct, i));
      element->dirty = TRUE;      
    }

  return ;
}
static void initAttributes(GArray *array)
{
  ZMapXMLAttribute attribute = NULL;
  int i;

  for(i = 0; i < array->len; i++)
    {
      attribute = &(g_array_index(array, zmapXMLAttributeStruct, i));
      attribute->dirty = TRUE;      
    }

  return ;
}

/* getOffendingXML - parser, context - Returns a string (free it!)

* This returns a string from the current input context which will
* roughly be 1 tag or context bytes either side of the current
* position.  

* e.g. getOffendingXML(parser, 10) context = <alpha><beta><gamma>...
* will return <alpha><beta><gamma> if current position is start beta
*/
/* If return is non null it needs freeing sometime in the future! */
/* slightly inspired by xml parsers which display errors like
 * <this isnt="valid"><xml></uknow>
 * error occurred ----------^ line 101
 */
static char *getOffendingXML(ZMapXMLParser parser, int context)
{
  char *bad_xml = NULL;
  const char *tmpCntxt = NULL;
  char *tmp = NULL, *end = NULL;
  int curr = 0, size = 0, byte = 0;

  if((tmpCntxt = XML_GetInputContext(parser->expat, &curr, &size)) != NULL)
    {
      byte  = curr; //XML_GetCurrentColumnNumber(parser->expat);
      if(byte > 0 && curr && size)
        {
          if(byte + context < size)
            {
              tmp = (char *)(byte + tmpCntxt + context);
              while(*tmp != '\0' && *tmp != '>'){ tmp++; }
              end = tmp;
            }
          else
            end = (char *)(size + tmpCntxt - 1);

          if(byte > context)
            {
              tmp = (char *)(byte + tmpCntxt - context);
              while(*tmp != '<'){ tmp--; }
            }
          else
            tmp = (char  *)tmpCntxt;

          bad_xml = g_strndup(tmp, end - tmp + 1);
        }
      else                      /* There's not much we can do here */
        bad_xml = g_strndup(tmpCntxt, context);
    }

  return bad_xml;               /* PLEASE free me later */
}

static void abortParsing(ZMapXMLParser parser, char *reason, ...)
{
  enum XML_Status stop_status;
  va_list args;
  char *error;
#ifdef ZMAP_USING_EXPAT_1_95_8_OR_ABOVE
  XML_ParsingStatus status;

  XML_GetParsingStatus(parser->expat, &status);

  /* We can only Stop if we're parsing! */
  if(status.parsing == XML_PARSING)
#endif 
    stop_status = XML_StopParser(parser->expat, FALSE);

  if(!(parser->aborted_msg))    /* So we only see the first error, not so we only see the last */
    {
      va_start(args, reason);
      error = g_strdup_vprintf(reason, args);
      va_end(args);

      parser->aborted_msg = g_strdup( error );
  
      g_free(error);
    }

  return ;
}

static void setupAutomagicCleanup(ZMapXMLParser parser, ZMapXMLElement element)
{
  ZMapXMLMarkupObjectHandler user_handler = NULL;
  tagHandlerItem automagic = NULL;
  
  if(!(user_handler = getObjHandler(element, parser->endTagHandlers)))
    {
      automagic          = g_new0(tagHandlerItemStruct, 1);
      automagic->id      = element->name;
      automagic->handler = defaultAlwaysTrueHandler;
      parser->endTagHandlers = 
        g_list_append(parser->endTagHandlers, automagic);
    }

  return ;
}

static gboolean defaultAlwaysTrueHandler(void *ignored_data,
                                         ZMapXMLElement element,
                                         ZMapXMLParser parser)
{
  return TRUE;
}
